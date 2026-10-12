"""SD-side helper library. Runs inside Substance Designer via execute_code.

Written against the SD 16 API directly: the bundled plugin calls
`setPropertyValueFromId`, which no longer exists, so none of its parameter
sets land. Everything here goes through getPropertyFromId + setPropertyValue.

Every operation is appended to a log file, because a hard failure in the C++
bindings kills the captured stdout and the response comes back empty.
"""

import os
import traceback

import sd
from sd.api.sdproperty import SDPropertyCategory
from sd.api.sbs.sdsbscompgraph import SDSBSCompGraph
from sd.api.sdvaluefloat import SDValueFloat
from sd.api.sdvaluefloat2 import SDValueFloat2
from sd.api.sdvaluefloat3 import SDValueFloat3
from sd.api.sdvaluefloat4 import SDValueFloat4
from sd.api.sdvalueint import SDValueInt
from sd.api.sdvalueint2 import SDValueInt2
from sd.api.sdvaluebool import SDValueBool
from sd.api.sdvaluecolorrgba import SDValueColorRGBA
from sd.api.sdbasetypes import float2, float3, float4, int2, ColorRGBA

LIB = "C:/Program Files/Adobe/Adobe Substance 3D Designer/resources/packages/"
LOGPATH = "C:/Users/ethan/AppData/Local/Temp/claude/sdprobe/build.log"

_app = sd.getContext().getSDApplication()
_pm = _app.getPackageMgr()
_pkg_cache = {}


def log_reset():
    os.makedirs(os.path.dirname(LOGPATH), exist_ok=True)
    open(LOGPATH, "w", encoding="utf-8").close()


def log(msg):
    with open(LOGPATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


def lib_resource(ident, filename=None):
    fn = filename or (ident + ".sbs")
    if fn not in _pkg_cache:
        path = LIB + fn
        if not os.path.exists(path):
            raise RuntimeError("missing library file: " + path)
        _pkg_cache[fn] = _pm.loadUserPackage(path, updatePackages=False)
    for r in _pkg_cache[fn].getChildrenResources(True):
        try:
            if r.getIdentifier() == ident:
                return r
        except Exception:
            pass
    raise RuntimeError("resource %s not found in %s" % (ident, fn))


def _as_seq(v, n):
    if isinstance(v, (list, tuple)):
        out = list(v)[:n]
        while len(out) < n:
            out.append(out[-1] if out else 0.0)
        return [float(x) for x in out]
    return [float(v)] * n


def prop_set(node, pid, value, category=SDPropertyCategory.Input):
    """Set a parameter, building the SDValue subclass the property expects."""
    prop = node.getPropertyFromId(pid, category)
    if prop is None:
        log("    skip %s (no such property)" % pid)
        return False
    cur = node.getPropertyValue(prop)
    cls = type(cur).__name__ if cur is not None else "?"
    if cls == "SDValueFloat":
        val = SDValueFloat.sNew(float(value))
    elif cls == "SDValueFloat2":
        val = SDValueFloat2.sNew(float2(*_as_seq(value, 2)))
    elif cls == "SDValueFloat3":
        val = SDValueFloat3.sNew(float3(*_as_seq(value, 3)))
    elif cls == "SDValueFloat4":
        val = SDValueFloat4.sNew(float4(*_as_seq(value, 4)))
    elif cls == "SDValueColorRGBA":
        val = SDValueColorRGBA.sNew(ColorRGBA(*_as_seq(value, 4)))
    elif cls == "SDValueInt":
        val = SDValueInt.sNew(int(value))
    elif cls == "SDValueInt2":
        s = _as_seq(value, 2)
        val = SDValueInt2.sNew(int2(int(s[0]), int(s[1])))
    elif cls == "SDValueBool":
        val = SDValueBool.sNew(bool(value))
    else:
        log("    skip %s (unhandled value type %s)" % (pid, cls))
        return False
    node.setPropertyValue(prop, val)
    return True


def out_ids(node):
    return [p.getId() for p in node.getProperties(SDPropertyCategory.Output)]


def in_ids(node):
    return [p.getId() for p in node.getProperties(SDPropertyCategory.Input)
            if not p.getId().startswith("$")]


def connect(src, dst, in_id=None, out_id=None):
    """Connect, resolving port names first.

    Atomics expose `unique_filter_output`, but an instance node's ports are
    named after the instanced graph (`Fiber`, `Clouds_2`, `R`/`G`/`B`/`A`).
    Passing an ID that does not exist takes down the whole SD Python call with
    no traceback, so both ends are resolved and validated before connecting.
    """
    outs = out_ids(src)
    if out_id is None:
        out_id = "unique_filter_output" if "unique_filter_output" in outs else outs[0]
    if out_id not in outs:
        raise RuntimeError("output %r not in %s" % (out_id, outs))

    ins = in_ids(dst)
    if in_id is None:
        in_id = ins[0]
    if in_id not in ins:
        # instance inputs are capitalised; try a case-insensitive match
        match = [i for i in ins if i.lower() == str(in_id).lower()]
        if not match:
            raise RuntimeError("input %r not in %s" % (in_id, ins))
        in_id = match[0]
    src.newPropertyConnectionFromId(out_id, dst, in_id)


class Graph(object):
    def __init__(self, name, pkg=None):
        self.pkg = pkg or _pm.newUserPackage()
        self.graph = SDSBSCompGraph.sNew(self.pkg)
        self.graph.setIdentifier(name)
        self.name = name
        self.nodes = []

    def atomic(self, kind, **params):
        log("  atomic %s" % kind)
        n = self.graph.newNode("sbs::compositing::" + kind)
        self.nodes.append(n)
        for k, v in params.items():
            log("    set %s=%r" % (k, v))
            prop_set(n, k, v)
        return n

    def inst(self, ident, filename=None, **params):
        log("  inst %s" % ident)
        n = self.graph.newInstanceNode(lib_resource(ident, filename))
        self.nodes.append(n)
        for k, v in params.items():
            log("    set %s=%r" % (k, v))
            prop_set(n, k, v)
        return n

    def mix(self, fg, bg, amount=1.0, mask=None):
        n = self.atomic("blend", opacitymult=amount)
        connect(fg, n, "source")
        connect(bg, n, "destination")
        if mask is not None:
            connect(mask, n, "opacity")
        return n

    def out(self, src):
        o = self.graph.newNode("sbs::compositing::output")
        connect(src, o, "inputNodeOutput")
        self.nodes.append(o)
        return o

    def set_size(self, log2=10, fmt=0):
        """Set resolution once on the graph, as an absolute value.

        `$outputsize` defaults to relative-to-parent, where the value is an
        exponent *offset*, so writing 10 to every node multiplies the
        resolution by 1024 instead of setting it to 1024px. Set it absolute on
        the graph and let the nodes inherit.
        """
        from sd.api.sdproperty import SDPropertyInheritanceMethod
        log("  set_size %d (absolute, graph level)" % log2)
        prop = self.graph.getPropertyFromId("$outputsize",
                                            SDPropertyCategory.Input)
        if prop is None:
            log("    graph has no $outputsize")
            return
        self.graph.setPropertyInheritanceMethod(
            prop, SDPropertyInheritanceMethod.Absolute)
        self.graph.setPropertyValue(prop, SDValueInt2.sNew(int2(log2, log2)))

        # 8 bits per channel; SD defaults normals to 16-bit, which quadruples
        # file size for no benefit in UE
        fprop = self.graph.getPropertyFromId("$format", SDPropertyCategory.Input)
        if fprop is not None:
            self.graph.setPropertyInheritanceMethod(
                fprop, SDPropertyInheritanceMethod.Absolute)
            self.graph.setPropertyValue(fprop, SDValueInt.sNew(int(fmt)))

        # nodes inherit $format from their *input*, not the graph, so a
        # graph-level setting alone leaves normals at 16-bit
        for n in self.nodes:
            fp = n.getPropertyFromId("$format", SDPropertyCategory.Input)
            if fp is None:
                continue
            try:
                n.setPropertyInheritanceMethod(
                    fp, SDPropertyInheritanceMethod.Absolute)
                n.setPropertyValue(fp, SDValueInt.sNew(int(fmt)))
            except Exception as e:
                log("    format skip: %s" % e)

    def compute(self):
        log("  compute")
        self.graph.compute()


def save_node(node, path):
    for p in node.getProperties(SDPropertyCategory.Output):
        v = node.getPropertyValue(p)
        if v is None:
            continue
        tex = v.get()
        if hasattr(tex, "save"):
            os.makedirs(os.path.dirname(path), exist_ok=True)
            tex.save(path)
            return os.path.exists(path)
    return False

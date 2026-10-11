import sys, io, json, time
sys.path.insert(0, ".")
from sd import call
lib = io.open("sdlib.py", encoding="utf-8").read()
body = io.open("build2.py", encoding="utf-8").read()
for target in sys.argv[1:]:
    code = lib + ('\nTARGET = "%s"\n' % target) + body
    t0 = time.time()
    try:
        r = call("execute_code", {"code": code}, timeout=175)
        ok = r.get("result", {}).get("executed")
        print("%-15s %6.1fs executed=%s" % (target, time.time()-t0, ok))
    except Exception as e:
        print("%-15s %6.1fs CALL-EXC %s" % (target, time.time()-t0, e))
    try:
        tail = io.open("C:/Users/ethan/AppData/Local/Temp/claude/sdprobe/build.log",
                       encoding="utf-8").read().strip().splitlines()
        print("     ", [l for l in tail if l.startswith(("RESULT", "EXC"))][:1] or tail[-1:])
    except Exception:
        pass

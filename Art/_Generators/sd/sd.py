"""Minimal client for the Substance Designer MCP plugin's TCP command port."""
import json
import socket
import struct

HOST, PORT = "localhost", 9881


def call(cmd_type, params=None, timeout=120.0):
    payload = json.dumps({"type": cmd_type, "params": params or {}}).encode()
    with socket.create_connection((HOST, PORT), timeout=timeout) as s:
        s.settimeout(timeout)
        s.sendall(struct.pack(">I", len(payload)) + payload)
        head = b""
        while len(head) < 4:
            c = s.recv(4 - len(head))
            if not c:
                raise ConnectionError("closed reading header")
            head += c
        n = struct.unpack(">I", head)[0]
        buf = b""
        while len(buf) < n:
            c = s.recv(n - len(buf))
            if not c:
                raise ConnectionError("closed reading body")
            buf += c
    return json.loads(buf.decode())

#!/usr/bin/env python3
"""
HTTPS interface for handling CDR client-server requests

Trap all exceptions except those for imports from the standard library.
"""

import datetime
import os
import sys

try:
    WRITER = sys.stdout.buffer
except:
    import msvcrt
    WRITER = sys.stdout
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
    msvcrt.setmode(sys.stdin.fileno(), os.O_BINARY)

try:
    from cdr_commands import CommandSet
    responses = CommandSet().get_responses()
    WRITER.write(b"Content-type: application/xml\r\n\r\n")
    WRITER.write(responses)
except Exception as e:
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
    try:
        for drive in "DCEF":
            path = drive + ":/cdr/Log"
            if os.path.exists(path):
                with open(path + "/https_api.log", "a") as fp:
                    fp.write("{} [ERROR] {}\n".format(now, e))
                break
    except:
        pass
    WRITER.write(b"Status: 500 CDR unavailable\r\n\r\n")

#!/usr/bin/env python3
"""
HTTPS interface for handling CDR client-server requests

Trap all exceptions except those for imports from the standard library.
"""

import datetime
import os
import sys


try:
    from cdr_commands import CommandSet
    responses = CommandSet().get_responses()
    sys.stdout.buffer.write(b"Content-type: application/xml\r\n\r\n")
    sys.stdout.buffer.write(responses)
except Exception as e:
    now = datetime.datetime.now()
    try:
        for drive in "DCEF":
            path = f"{drive}:/cdr/Log"
            if os.path.exists(path):
                with open(f"{path}/https_api.log", "a") as fp:
                    fp.write(f"{now} [ERROR] {e}\n")
                break
    except Exception:
        pass
    sys.stdout.buffer.write(b"Status: 500 CDR unavailable\r\n\r\n")

#!/usr/bin/env python3
"""
HTTPS interface for handling CDR client-server requests
Trap all exceptions.
"""

import datetime
import os
import sys

try:
    from cdr_command_set import CommandSet
    responses = CommandSet().get_responses()
    sys.stdout.buffer.write(b"Content-type: application/xml\r\n\r\n")
    sys.stdout.buffer.write(responses)
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
    sys.stdout.buffer.write(b"Status: 500 CDR unavailable\r\n\r\n")

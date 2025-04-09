#!/usr/bin/env python3
"""
HTTPS interface for handling CDR client-server requests

Trap all exceptions except those for imports from the standard library.
"""

import datetime
import sys


try:
    from cdr_commands import CommandSet
    responses = CommandSet().get_responses()
    sys.stdout.buffer.write(b"Content-type: application/xml\r\n\r\n")
    sys.stdout.buffer.write(responses)
except Exception as e:
    now = datetime.datetime.now()
    try:
        from cdrapi.settings import Tier
        tier = Tier()
        path = f"{tier.logdir}/https_api.log"
        with open(path, "a", encoding="utf-8") as fp:
            fp.write(f"{now} [ERROR] {e}\n")
    except Exception:
        pass
    sys.stdout.buffer.write(b"Status: 500 CDR unavailable\r\n\r\n")

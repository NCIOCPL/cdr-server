#!/usr/bin/env python

"""Identify loader JSON files which have been hand-edited.

We try to store the serialized JSON files used by the loader as the
standard library would print it using 2-space indents. This makes
it easier to get useful output from `git diff` to identify the
real changes. Run this from the directory in which this script lives
(the parent directory of the Loader subdirectory).
"""

import json
import glob

FILL = "."

for name in glob.glob("Loader/*.json"):
    with open(name) as fp:
        json_string = fp.read().strip()
        try:
            values = json.loads(json_string)
        except Exception:
            print(f"{name:{FILL}<20} malformed")
            continue
        else:
            if json_string == json.dumps(values, indent=2):
                print(f"{name:{FILL}<20} ok")
            else:
                print(f"{name:{FILL}<20} hand-edited")

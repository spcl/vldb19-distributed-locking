#!/usr/bin/env python3

import sys
import json
import argparse

# Setup parser for command line arguments
parser = argparse.ArgumentParser()
parser.add_argument('offset', type=int)
args = parser.parse_args()

i = 0

for line in sys.stdin:
    data = json.loads(line)
    data['lock_id'] = i + args.offset
    i += 1
    print(json.dumps(data))

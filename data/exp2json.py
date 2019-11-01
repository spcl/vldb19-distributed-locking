#!/usr/bin/env python3

import argparse
import json

parser = argparse.ArgumentParser(description='Convert experiment output (.exp) to json format.')
parser.add_argument('-i', '--input', help='Input .exp file')
parser.add_argument('-c', '--common', type=str, default='{}',
                    help='JSON object with common keys for the whole file.')
args = parser.parse_args()

d = None
with open(args.input, 'r') as f:
    for line in f:
        if line.strip() == '####':
            if d is not None:
                print(json.dumps(d))
            d = json.loads(args.common)
            d['__filename'] = args.input
        else:
            k, v = line.strip().split()
            d[k] = v
if d is not None:
    print(json.dumps(d))

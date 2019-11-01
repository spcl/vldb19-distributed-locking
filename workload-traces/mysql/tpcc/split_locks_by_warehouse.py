#!/usr/bin/env python3

import argparse
import json
import gzip
import sys

# Setup parser for command line arguments
parser = argparse.ArgumentParser()
parser.add_argument('prefix')
args = parser.parse_args()

outputfile = gzip.open('/dev/null')
current_warehouse = None

with open('{0}.split-locks'.format(args.prefix), 'w') as f:
    f.write('')

for line in sys.stdin:
    data = json.loads(line)
    warehouse = data['ware_house']
    if warehouse != current_warehouse:
        outputfile.close()
        outputname = '{0}-wh{1:06}.json.gz'.format(args.prefix, warehouse)
        outputfile = gzip.open(outputname, mode='wt')
        current_warehouse = warehouse
    outputfile.write(line)

outputfile.close()

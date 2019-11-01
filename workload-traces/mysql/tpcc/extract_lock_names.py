#!/usr/bin/env python3

import fileinput
import json
from collections import OrderedDict

locks = set()

for line in fileinput.input():
    data = json.loads(line)

    # Get only lock events
    if data.get('action') not in ['lock', 'unlock']:
        continue

    # Insert ware house where missing (home ware house by default)
    ware_house = data['ware_house'] if 'ware_house' in data else data['home_ware_house']
    ware_house = int(ware_house)

    locks.add((ware_house, data['object_name']))

for l in locks:
    data = OrderedDict([('ware_house', l[0]), ('object_name', l[1])])
    print(json.dumps(data))

#!/usr/bin/env python3

import json
import fileinput

#current_trx_num = None
#locks_of_current_trx = set()

# Read trace events one by one
for line in fileinput.input():
    data = json.loads(line)

    # Get only lock events
    if data.get('action') != 'lock':
        continue

    # Extract interesting properties
    object_name = data['object_name']
    lock_mode = data['lock_mode']
    trx_name = data['trx_name']
    trx_num = data['trx_num']

    # Insert ware house where missing (home ware house by default)
    ware_house = data['ware_house'] if 'ware_house' in data else data['home_ware_house']
    ware_house = int(ware_house)

#    # Skip over duplicate locks of the same TRX
#    if trx_num != current_trx_num:
#        locks_of_current_trx.clear()
#        current_trx_num = trx_num
#
#    if lock_id in locks_of_current_trx:
#        continue
#    locks_of_current_trx.add(lock_id)

    # Output result
    print(json.dumps({ 'ware_house': ware_house,
                       'object_name': object_name,
                       'lock_mode': lock_mode,
                       'trx_name': trx_name,
                       'trx_num': trx_num }))

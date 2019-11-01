#!/usr/bin/env python3

import fileinput
import re
import json

bootup_stripped = False

for line in fileinput.input():
    if line.strip().endswith('ready for connections.'):
        bootup_stripped = True
        continue

    if not bootup_stripped:
        continue

    if line.startswith('UNLOCK RECORD ') or line.startswith('RECORD '):
        result = re.match('(UNLOCK )?RECORD LOCK space ([0-9]*) page_no ([0-9]*) heap_no ([0-9]*) (.*?(/\\* Partition `p([0-9]*)` \\*/)?) trx id ([0-9]*) lock_mode ([A-Z]*)( ORDINARY)?( GAP)?( REC_NOT_GAP)?( INSERT_INTENTION)?', line)

        if not result:
            print("Warning: line not in expected format: " + line)
            continue

        action = 'unlock' if result.group(1) else 'lock'
        space_no = int(result.group(2))
        page_no = int(result.group(3))
        heap_no = int(result.group(4))
        object_name = result.group(5)
        partition_no = int(result.group(7))
        trx_id = int(result.group(8))
        lock_mode = result.group(9)
        ordinary = True if result.group(10) else False
        gap = True if result.group(11) else False
        recnotgap = True if result.group(12) else False
        insertint = True if result.group(13) else False

        print(json.dumps({ 'action' : action,
                           'type': 'record',
                           'space_no' : space_no,
                           'page_no' : page_no,
                           'heap_no' : heap_no,
                           'object_name' : object_name,
                           'partition_no' : partition_no,
                           'trx_id' : trx_id,
                           'lock_mode' : lock_mode,
                           'ordinary' : ordinary,
                           'gap' : gap,
                           'recnotgap' : recnotgap,
                           'insertint' : insertint }))

    if line.startswith('UNLOCK TABLE ') or line.startswith('TABLE '):
        # TABLE LOCK table `tpcc10`.`warehouse` /* Partition `p5` */ trx id 4072790 lock_mode IX
        result = re.match('(UNLOCK )?TABLE LOCK (.*?(/\\* Partition `p([0-9]*)` \\*/)?) trx id ([0-9]*) lock_mode ([-A-Z]*)', line)

        if not result:
            print("Warning: line not in expected format: " + line)
            continue

        action = 'unlock' if result.group(1) else 'lock'
        object_name = result.group(2)
        partition_no = int(result.group(4))
        trx_id = int(result.group(5))
        lock_mode = result.group(6)

        print(json.dumps({ 'action' : action,
                           'type': 'table',
                           'object_name' : object_name,
                           'partition_no' : partition_no,
                           'trx_id' : trx_id,
                           'lock_mode' : lock_mode }))

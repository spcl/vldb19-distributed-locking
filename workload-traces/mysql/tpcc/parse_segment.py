#!/usr/bin/env python3

import fileinput
import re
import json
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

class PropertyStack:
    def __init__(self):
        self.Clear()

    def Clear(self):
        self.current_depth = 0
        self.property_stack = [{}]

    def Push(self):
        self.current_depth += 1
        self.property_stack.append({})

    def Pop(self):
        if self.current_depth > 0:
            self.current_depth -= 1
            self.property_stack.pop()
        else:
            eprint("Warning: unbalanced start/stop: Pop from empty stack")

    def Add(self, name, value):
        self.property_stack[-1][name] = value

    def Properties(self):
        ret = {}
        for d in self.property_stack:
            ret.update(d)
        return ret

def ParseIntOrDefault(i, default):
    try:
        return int(i)
    except:
        return default

def ParseLine(property_stack, line):
    if line.startswith('UNLOCK RECORD ') or line.startswith('RECORD '):
        result = re.match('(UNLOCK )?RECORD LOCK (space ([0-9]*) page_no ([0-9]*) heap_no ([0-9]*) (.*?(/\\* Partition `p([0-9]*)` \\*/)?)) trx id ([0-9]*) lock_mode ([A-Z]*)( ORDINARY)?( GAP)?( REC_NOT_GAP)?( INSERT_INTENTION)?', line)

        if not result:
            eprint("Warning: line not in expected format: " + line)
            return

        property_stack.Add('action', 'unlock' if result.group(1) else 'lock')
        property_stack.Add('object_name', result.group(2))
        property_stack.Add('space_no', int(result.group(3)))
        property_stack.Add('page_no', int(result.group(4)))
        property_stack.Add('heap_no', int(result.group(5)))
        property_stack.Add('table_name', result.group(6))
        property_stack.Add('object_type', 'record')
        property_stack.Add('partition_no', ParseIntOrDefault(result.group(8), -1))
        property_stack.Add('trx_id', int(result.group(9)))
        property_stack.Add('lock_mode', result.group(10))
        property_stack.Add('ordinary', True if result.group(11) else False)
        property_stack.Add('gap', True if result.group(12) else False)
        property_stack.Add('recnotgap', True if result.group(13) else False)
        property_stack.Add('insertint', True if result.group(14) else False)

    if line.startswith('UNLOCK TABLE ') or line.startswith('TABLE '):
        result = re.match('(UNLOCK )?TABLE LOCK (.*?(/\\* Partition `p([0-9]*)` \\*/)?) trx id ([0-9]*) lock_mode ([-A-Z]*)', line)

        if not result:
            eprint("Warning: line not in expected format: " + line)
            return

        property_stack.Add('action', 'unlock' if result.group(1) else 'lock')
        property_stack.Add('object_name', result.group(2))
        property_stack.Add('object_type', 'table')
        property_stack.Add('partition_no', ParseIntOrDefault(result.group(4), -1))
        property_stack.Add('trx_id', int(result.group(5)))
        property_stack.Add('lock_mode', result.group(6))

    if line.startswith('start: trx'):
        result = re.match('start: trx ([a-z]*) on home w_id ([0-9]*) trx_num ([0-9]*)', line)
        if not result:
            eprint('Warning: could not parse line: ' + line);
            return
        property_stack.Add('trx_name', result.group(1))
        property_stack.Add('home_ware_house', int(result.group(2)))
        property_stack.Add('trx_num', int(result.group(3)))

    elif 'start' in line and 'ware house' in line:
        result = re.match('.* ware house ([0-9]+).*', line)
        if not result:
            eprint('Warning: could not parse line: ' + line);
            return
        property_stack.Add('ware_house', int(result.group(1)))

property_stack = PropertyStack()

for line in fileinput.input():

    if line.startswith('end: '):
        property_stack.Pop()

    # Fall-back in case a 'start' misses a matching 'end'
    if line.startswith('start: trx'):
        if property_stack.current_depth != 0:
            eprint("Warning: start of transaction found at depth != 0!")
        property_stack.Clear()

    ParseLine(property_stack, line)

    if line.startswith('start: '):
        property_stack.Push()

    print(json.dumps(property_stack.Properties()))

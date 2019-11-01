#!/usr/bin/env python3

import fileinput
import lzma
import re

with open('dummy.out', 'w') as f:
    f.write('')

class Segment:
    def __init__(self):
        self.num = 0;
        self.content = []
        self.name = "unknown"
        self.files = {}

    def Flush(self):
        filename = '{0}.out.xz'.format(self.name)
        if filename not in self.files:
            self.files[filename] = lzma.open(filename, mode='wt')
        f = self.files[filename]
        for l in self.content:
            f.write(l)
        self.content = []
        self.num += 1
        self.name = "unknown"

    def Append(self, line):
        self.content.append(line)

    def SetName(self, name):
        self.name = name

trx_start_re = re.compile('start: trx ([a-z]*) on home w_id ([0-9]*)')

current_segment = Segment()
for line in fileinput.input():
    result = trx_start_re.match(line)
    if result:
        current_segment.Flush()
        current_segment.SetName("wh{0:06}".format(int(result.group(2))))
        line = line.strip() + " trx_num {0}\n".format(current_segment.num)

    current_segment.Append(line)

current_segment.Flush()

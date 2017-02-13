#!/usr/bin/env python
import hyperdex.admin
a = hyperdex.admin.Admin('192.168.0.170', 1982)

a.add_space('''
space usertable 
key k
attributes field0, field1, field2, field3, field4, field5, field6, field7, field8, field9
create 24 partitions
tolerate 1 failure
''')

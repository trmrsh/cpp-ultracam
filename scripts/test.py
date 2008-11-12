#!/usr/bin/env python

class Test(dict):

    def __init__(self):
        dict.__init__(self)
        self['Name'] = 'Tom'

    def __setitem__(self, key, value):
        dict.__setitem__(self, key.upper(), value)

    def name(self):
        return self['Name']

x = Test()

print x

x['a'] = 2

print x

x.name()

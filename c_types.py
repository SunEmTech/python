# -*- coding: utf-8 -*-
"""
Created on Sat Apr 11 15:32:18 2020

@author: sandverm
"""
from ctypes import *

class NetStruct(BigEndianStructure):
    _pack_ = 1

    def raw(self):
        return bytes(memoryview(self))

    def __new__(self, sb=None):
        if sb:
            return self.from_buffer_copy(sb)
        else:
            return BigEndianStructure.__new__(self)

    def __init__(self, sb=None):
        pass

class NestedStruct(NetStruct):
    _fields_ = [('flags', c_ubyte*3),
                ('val1', c_ubyte),
                ('val2', c_ubyte)]

class ExampleNetworkPacket(NetStruct):
    _fields_ = [('version', c_ushort),
                ('reserved', c_ushort),
                ('sanity', c_uint),
                ('ns', NestedStruct),
                ('datalen', c_uint)]
    _data = (c_ubyte * 0)()

    @property
    def data(self):
        return memoryview(self._data)

    @data.setter
    def data(self, indata):
        self.datalen = len(indata)
        self._data = (self._data._type_ * len(indata))()
        memmove(self._data, indata, len(indata))    

    def raw(self):
        return super(self.__class__, self).raw() + self.data


enp = ExampleNetworkPacket()
enp.ns.flags[0] = 1
enp.ns.flags[1] = 1
enp.ns.val2 = 0xff
enp.data = b"Hello world!"

print(enp.raw())

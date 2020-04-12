# -*- coding: utf-8 -*-
"""
Created on Tue May 21 12:26:56 2019

@author: sandverm
"""

 ### two_tasks.py
from time import sleep

def foo():
    for i in range(10):
        print("In foo: counting", i)
        yield
        sleep(0.5)


def bar():
    for i in range(10):
        print("In bar: counting", i)
        yield
        sleep(0.5)


t1 = foo()
t2 = bar()
#tasks = [t1, t2]
#while tasks:
#    for t in tasks:
#        try:
#            next(t)
#        except:
#            tasks.remove(t)


#from collections import deque
#tasks = deque([t1, t2])
#while tasks:
#    try:
#        t = tasks[0]
#        try:
#            next(t)
#        except StopInteration:
#            tasks.popleft()
#        else:
#            tasks.rotate(-1)

with Runqueue() as rq:
    rq.add(foo)
    rq.add(bar)
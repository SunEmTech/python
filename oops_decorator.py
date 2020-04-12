# -*- coding: utf-8 -*-
"""
Created on Tue May 21 15:21:49 2019

@author: sandverm
"""

def style(s):
    def decorate(fn):
        if s == "upper":
            def wrap():
                return fn().upper()
        elif s == "italics":
            def wrap():
                return "<i>" + fn() + "</i>"
            
        return wrap
    return decorate

@style("italics") # Parameterized decorator
@style("upper")
def greet():
    return "Hello world"

greet()
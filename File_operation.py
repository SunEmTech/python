# -*- coding: utf-8 -*-
"""
Created on Sun Mar 17 13:42:16 2019

@author: sandverm
"""

import os
print (os.getcwd())

token = dict()

file1= open("p2p_main.c", "r")
for fdata in file1.readlines():
    data = fdata.replace("\n", " ").split(" ")
    for tok in data:
        if tok in ["{", "}", "", " ", "return", ";", "#endif", "if"]:
            continue
        #if tok[-1:] is "(":
        #print(tok)
        if token.get(fdata) is None:
            token[fdata] = set()
        token[fdata].add(file1.name)
    
#print(token.)
for i in token.keys():
    print(token, "  :", token[i], "\n")


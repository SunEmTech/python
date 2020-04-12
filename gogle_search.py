# -*- coding: utf-8 -*-
"""
Created on Tue Sep  3 09:59:16 2019

@author: sandverm
"""

try: 
    from googlesearch import search 
except ImportError:  
    print("No module named 'google' found") 
  
# to search 
query = "A computer science portal"
  
for j in search(query, tld="co.in", num=10, stop=1, pause=2): 
    print(j) 
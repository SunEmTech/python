# -*- coding: utf-8 -*-
"""
Created on Sun Sep 15 22:57:09 2019

@author: sandverm
"""

import numpy as np

RC = 5

chess = np.zeros((RC))

def fill_random(mat):
    import random
    
    for i in range(RC):
        num = random.randrange(1, RC+1, 1)
        mat[i] = num

fill_random(chess)

print(chess)
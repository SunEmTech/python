# -*- coding: utf-8 -*-
"""
Created on Sun Sep 15 11:04:34 2019

@author: sandverm
"""

RC = 9
import numpy as np
mat = np.zeros((RC,RC))

def get_uniqu(mat, row, clm):
    import random
    # Try Random number for Max limit
    if mat[row][clm] != 0:
        return mat[row][clm]
    
    limit = RC * 2
    while limit:
        limit -= 1
        if limit is 0:
            return 0
        num = random.randrange(1, RC+1, 1)
        #check in Row
        if num in mat[row]:
            continue
        #check in Clm
        if num in mat[:, [clm]]:
            continue
        return num

def fill_random_number(mat):
    # loop for row
    proper = True
    for i in range(RC):
        # loop for column
        for j in range(RC):
            mat[i][j] = get_uniqu(mat, i, j)
            if mat[i][j] == 0:
                proper = False
            #print("mat[{}][{}] = {}".format(i, j, mat[i][j]))
            #print (mat)
    return proper

def del_row_clm(mat):
    del_row_set = set()
    del_clm_set = set()
    for i in range(RC):
        if 0 in mat[i]:
            del_row_set.add(i)
        if 0 in mat[:, [i]]:
            del_clm_set.add(i)
    for i in del_row_set:
        mat[i] = 0
    for i in del_clm_set:
        mat[:, [i]] = 0
            
proper = False
count = 0
while proper is False:
    count += 1
    del_row_clm(mat)
    print(mat)
    # reset complete matrix
    #mat[:][:] = 0
    proper = fill_random_number(mat)
    print(mat)
    print("count: {}".format(count))

    #proper = True
# -*- coding: utf-8 -*-
"""
Created on Mon Apr  8 22:20:08 2019

@author: Sandeep
"""

import pandas as pd
import numpy as np


def entropy(d):      
    ones = d.sum(axis=0)
    row = d.count()
    zeros = row-ones

    Entropy= -(zeros/row)*np.log2(zeros/row) -(ones/row)*np.log2(ones/row)
    E = Entropy.fillna(0)
    return float(E)

def IGain(clmn, res):
    ones_row_id = [i for i , n in enumerate(clmn) if n==1]
    zeros_row_id = [i for i , n in enumerate(clmn) if n==0]
    ones_row_cnt = len(ones_row_id)
    zeros_row_cnt = len(zeros_row_id)
    total_cnt = clmn.count()
   # print("Total_cnt {}, ones_cnt {}, zero_cnt {}".format(total_cnt, ones_row_cnt, zeros_row_cnt))
    c_l = pd.DataFrame(data=res, index=ones_row_id, dtype=int)
    c_r = pd.DataFrame(data=res, index=zeros_row_id, dtype=int)
    
    e_p = entropy(res)
    e_l = entropy(c_l)
    e_r = entropy(c_r)
#    print("entropy of parent {}, l_child: {}, r_child: {}".format(e_p, e_l, e_r)) 
    ig = e_p - (ones_row_cnt/total_cnt)*e_l - (zeros_row_cnt/total_cnt)*e_r
#    print("Information Gain: {}", ig)
    return ig




BITS_ID = "2018HTXXXXX"
number_of_files = 56
writer1 = pd.ExcelWriter(BITS_ID+'_infogain.xlsx', engine='xlsxwriter')
for i in range(1,number_of_files+1):
    csv_file = "data/"+str(i)+".csv"
    rawdata = df = pd.read_csv(csv_file, header=None)
    m=rawdata.median()
    dataset = pd.DataFrame(rawdata>m, dtype=int)
    dataset[20] = rawdata[20]
    #ig = main_IE(dataset)
    
    res = pd.DataFrame(data=dataset[20])
    ig = list()
    for f in range(20):
        clmn = dataset[f]
        ig.append(IGain(clmn, res))
    df = pd.DataFrame({"Information gain":ig}, index=range(1,21))
    df.to_excel(writer1, sheet_name=str(i)+"_csv")
    print(csv_file)
    print(df)
    
    
writer1.save()
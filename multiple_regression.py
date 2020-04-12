# -*- coding: utf-8 -*-
"""
Created on Sun Apr 21 21:45:40 2019

@author: sandverm
"""
import numpy as np
import pandas as pd

csv_file="data_asta/4)drug2.csv"
dataset = pd.read_csv(csv_file, names = ["sex", "dose", "response"])
ds = dataset.drop(index=0).reset_index(drop=True)
ds = pd.DataFrame(ds, dtype=float)
d = ds.head(200)

d=d.head(5)

Y = d["response"]
Y1 = [i for i in Y]
Y1 = np.array(Y1)
X = d.drop(columns="response")
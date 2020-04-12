# -*- coding: utf-8 -*-
"""
Created on Thu Apr 11 16:39:18 2019

@author: sandverm
"""
import numpy as np
import pandas as pd


#
#def find_line(data, num_col=1):
#    Y=
    
csv_file="data_asta/4)drug2.csv"
dataset = pd.read_csv(csv_file, names = ["sex", "dose", "response"])
ds = dataset.drop(index=0).reset_index(drop=True)
ds = pd.DataFrame(ds, dtype=float)
d = ds.head(200)
m = d[d["sex"] == 1].drop(columns="sex")
m = m.rename(columns= {"response": "response_m"})
f = d[d["sex"] == 0].drop(columns="sex")
f = f.rename(columns= {"response": "response_f"})

d_new = m.merge(f)

mean_m = d_new["response_m"].mean()
mean_f = d_new["response_f"].mean()
mean_t = pd.concat([d_new["response_m"], d_new["response_f"]]).mean()
mean_c = (mean_m+mean_f)/2

row_len = len(d_new)

SSC = np.square([mean_c-mean_m, mean_c-mean_f]).sum() * row_len  # Why Multiplication with row_len?

SST = np.square(mean_t-pd.concat([d_new["response_m"], d_new["response_f"]])).sum()

SSE = np.square(mean_m-d_new["response_m"]).sum() + np.square(mean_f-d_new["response_f"]).sum()

print("SST({}) = SSE({}) + SSC({}):({})".format(SST, SSE, SSC, SSE+SSC))
SSC_df = 1
SSE_df = row_len*2-2
SST_df = row_len-1

MSC = SSC/SSC_df
MSE = SSE/SSE_df
F = MSC/MSE










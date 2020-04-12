# -*- coding: utf-8 -*-
"""
Created on Thu Apr 11 16:39:18 2019

@author: Sandeep Verma
"""
import numpy as np
import pandas as pd

###################################################################################

def getF(*arg):
    import scipy.stats # To get F discribution value
    col_len = len(arg)
    row_len = len(arg[0])
    return scipy.stats.f.ppf(q=1-0.05, dfn=col_len-1, dfd=row_len-1)
    
    
def anova_one(*arg):
    import numpy as np
    col_mean = list()

    for data in arg:
        col_mean.append(data.mean())

    mean_t = np.concatenate([*arg]).mean()
    mean_c = sum(col_mean)/len(arg)
    
    col_len = len(arg[0])
    
    SSC = np.square([mean_c-mean for mean in col_mean]).sum() * col_len

    SST = np.square(mean_t-np.concatenate([col for col in arg])).sum()
    
    SSE = float()
    for col, mean in zip(arg, col_mean):
        SSE = SSE + np.square(mean-col).sum()
    
    print("SST({}) = SSE({}) + SSC({}):({})".format(SST, SSE, SSC, SSE+SSC))
    
    SSC_df = len(arg)-1
    SSE_df = col_len*len(arg)-len(arg)
    SST_df = col_len-1
    
    MSC = SSC/SSC_df
    MSE = SSE/SSE_df
    F = MSC/MSE
    return F

###############################################################################


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

F = anova_one( d_new["response_f"], d_new["response_m"])
F_critical = getF(d_new["response_f"], d_new["response_m"])

print("F_calculated: {}, F_critial: {}".format(F, F_critical))

if F > F_critical:
    print("Result: Population does not belong to same distribution")
else:
    print("Result: Population belongs to same distribution")


###############################################################################
## Using Library
#from scipy import stats
#
#F, p = stats.f_oneway(d_new["response_f"], d_new["response_m"])
#print(F)
#
#csv_file="data_asta/6)FuelEfficiency.csv"
#dataset = pd.read_csv(csv_file)# names = ["sex", "dose", "response"])
##ds = dataset.drop(index=0).reset_index(drop=True)
#ds = pd.DataFrame(ds, dtype=float)
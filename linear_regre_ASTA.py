# -*- coding: utf-8 -*-
"""
Created on Thu Apr 11 16:39:18 2019

@author: Sandeep
"""
import numpy as np
import pandas as pd

###############################################################################

def plot_regression_line(x, y, b): 
    import matplotlib.pyplot as plt
    # plotting the actual points as scatter plot 
    plt.scatter(x, y, color = "m", 
               marker = "o", s = 30) 
  
    # predicted response vector 
    y_pred = b[0] + b[1]*x 
  
    # plotting the regression line 
    plt.plot(x, y_pred, color = "g") 
  
    # putting labels 
    plt.xlabel('x') 
    plt.ylabel('y') 
  
    # function to show plot 
    plt.show()
    
def get_SSE(actual_class, predicted_class):
    #    for i, j in zip(actual_class, predicted_class):
    #        print("{} {}".format(i, j))
        
    sse = np.square(actual_class - predicted_class).sum()
    return sse

def get_Y(b, X):
    return b[0] + b[1] * X

def estimate_coef(x, y): 
    # number of observations/points 
    n = np.size(x) 
  
    # mean of x and y vector 
    m_x, m_y = np.mean(x), np.mean(y) 
  
    # calculating cross-deviation and deviation about x 
    SS_xy = np.sum(y*x) - n*m_y*m_x 
    SS_xx = np.sum(x*x) - n*m_x*m_x 
  
    # calculating regression coefficients 
    b_1 = SS_xy / SS_xx 
    b_0 = m_y - b_1*m_x 
  
    return(b_0, b_1)
    
def regression(clm_X, clm_Y):
    """
    y = c + ax
    sum(y) = n*a + b*sum(x)
    sum(xy) = sum(x)*a + b*sum(x2)
    Y=sum(y) X=sum(x)
    """
    sum_X = clm_X.sum()
    sum_Y = clm_Y.sum()
    
    sum_XY = clm_X.mul(clm_Y).sum()
    sum_XX = clm_X.mul(clm_X).sum()
    
    n = clm_X.count()
    
    a = (sum_Y * sum_X - n * sum_XY) / (sum_X * sum_X - n * sum_XX) 
    c = (sum_XX * sum_Y - sum_X * sum_XY) / (n * sum_XX - sum_X * sum_X)
    return (c, a)

###############################################################################

def anova_regression(actual_Y, predicted_Y):
    mean_Y = actual_Y.mean()
    SST = np.square(mean_Y-actual_Y).sum()
    SSE = np.square(actual_Y - predicted_Y).sum()
    SSR = SST-SSE
    DF = len(actual_Y) - 1;
    R_sqr = SSR/SST
    return R_sqr

###############################################################################
    
#Linear Regression
csv_file="data_asta/2)BlueGills.csv"
dataset = pd.read_csv(csv_file, names = ["age", "length"])
ds = dataset.drop(index=0).reset_index(drop=True)
ds = pd.DataFrame(ds, dtype=int)

#b = estimate_coef(ds["age"], ds["length"])
b = regression(ds["age"], ds["length"])
plot_regression_line(ds["age"], ds["length"], b)

precdicted_length = get_Y(b,ds["age"])
SSE = get_SSE (ds["length"], get_Y(b, ds["age"]))
print("SSE of the regression model: {}".format(SSE))
print("R_square {}".format(anova_regression(ds["length"], precdicted_length)))

age = 4.5
print("Predicted length of BlueGills of age {} is {}".format(age, get_Y(b, age)))











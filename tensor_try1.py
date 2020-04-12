# -*- coding: utf-8 -*-
"""
Created on Fri Sep 27 15:02:20 2019

@author: sandverm
"""

# -*- coding: utf-8 -*-
"""
Created on Thu Sep 26 20:38:01 2019

@author: sandverm
"""

import numpy as np # linear algebra
import pandas as pd # data processing, CSV file I/O (e.g. pd.read_csv)

import matplotlib.pyplot as plt

from sklearn.model_selection import train_test_split



import tensorflow as tf

drug = pd.read_csv("data_asta/4)drug2.csv")

X_train = drug.iloc[:, 0:2].values.astype('float32')
Y_train = drug.iloc[:, 2].values.astype('float32')

X = X_train
Y = Y_train
X_train, X_val, Y_train, Y_val = train_test_split(X_train, Y_train, test_size=0.10, random_state=42)

# Set parameters
learning_rate = 0.01
training_iteration = 30
batch_size = 100
display_step = 2
training_epochs = 100


# TF graph input
x = tf.placeholder(dtype='float32',shape=(None, 2))
y = tf.placeholder("float32") # 0-9 digits recognition => 10 classes

# Create a model

# Set model weights
W = tf.Variable(tf.zeros([2, 1]))
b = tf.Variable(tf.zeros([1]))

# Hypothesis 
y_pred = tf.add(tf.multiply(x, W), b) 
  
# Mean Squared Error Cost Function 
n = len(X_train)
cost = tf.reduce_sum(tf.pow(y_pred-y, 2)) / (2 * n) 
optimizer = tf.train.GradientDescentOptimizer(learning_rate).minimize(cost)
init = tf.global_variables_initializer()


# Starting the Tensorflow Session 
with tf.Session() as sess: 
      
    # Initializing the Variables 
    sess.run(init) 
      
    # Iterating through all the epochs 
    for epoch in range(training_epochs): 
          
        # Feeding each data point into the optimizer using Feed Dictionary 
       # for (_x, _y) in zip(X_train, Y_train): 
         #   sess.run(optimizer, feed_dict = {x : _x, y : _y}) 
        sess.run(optimizer, feed_dict = {x : X_train, y : Y_train})
        # Displaying the result after every 50 epochs 
        if (epoch + 1) % 10 == 0: 
            # Calculating the cost a every epoch 
            c = sess.run(cost, feed_dict = {x : X_train, y : Y_train}) 
            print("Epoch", (epoch + 1), ": cost =", c, "W =", sess.run(W), "b =", sess.run(b)) 
      
    # Storing necessary values to be used outside the Session 
    training_cost = sess.run(cost, feed_dict ={x: X_train, y: Y_train}) 
    weight = sess.run(W)
    bias = sess.run(b)




# -*- coding: utf-8 -*-
"""
Created on Fri Aug 30 22:01:40 2019

@author: sandverm

Cryptography  RC4 algo v0.1
"""

#user input
key = "password"
key = input("key:")
print (key)

key = [ord(key[i]) for i in range(len(key))]

BIT_LEN = 256

S = [s for s in range(BIT_LEN)]

T = [key[i%len(key)] for i in range(BIT_LEN)]

def swap(L, i, j):
    temp = L[i]
    L[i] = L[j]
    L[j] = temp

# Scrambling the value of T by permutation 
j = 0
for i in range(BIT_LEN):
    j = (j + S[i] + T[i]) % BIT_LEN
    swap(S, i, j)

def get_byte_stream():
    i = 0
    j = 0
    while True:
        i = (i + 1) % BIT_LEN
        j = (j + S[i]) % BIT_LEN
        swap(S, i, j)
        t = (S[i] + S[j]) % BIT_LEN
        k = S[t]
        yield k


# returns cipher
def encrypt(data, stream):
    cypher = []
    cypher = [chr((ord(d) ^ next(stream))) for d in data]
    return cypher

#data = [254, 247, 59]

data = input("data:")
print(data)

res = encrypt(data, get_byte_stream())

for i in res:
    print(i, end='')
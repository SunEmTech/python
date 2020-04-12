# -*- coding: utf-8 -*-
"""
Created on Wed May 22 16:23:58 2019

@author: sandverm
"""

#file = open("c:\Temp\wlan_vdev.h", "r")

def get_elements(file, struct_name):
    file = open(file, "r")
    data = file.readlines()
    
    for line in range(len(data)):
        if "struct" in data[line] and \
            struct_name in data[line] and \
            not(";" in data[line]):
            break
    
    print(data[line])

    while not("{" in data[line]):
        line += 1
    
    cnt = 0
    elements = []
    for line in range(line, len(data)):
        l = data[line]
        if l is "":
            continue
        if "{" in l:
            cnt += 1
            continue
        if "}" in l:
            cnt -= 1
        if cnt == 0:
            break
        if "#if" in l or "#endif" in l or "#else" in l or "struct" in l or \
            "union" in l or "};" in l or "{" in l or "#define" in l:
            continue
        l = l[:l.find(";")].strip()
        l = l[:l.find(":")].strip()
        if "/*" in l or "*/" in l or "//" in l:
            continue
        
        pair = l.split(" ", 1)
        if len(pair) == 1 and pair[0] is "":
            continue
        if len(pair) == 1:
            p = pair[0]
            #p = p[:p.find(":")].strip()
            pair[0] = "BOOL"
            pair.append(p)
        pair[1] = pair[1].strip()
        print(pair)
        #l.split(" ")
        elements.append(l)
        
        
    #print(elements)
#    for line in range(len(elements)):
#        l = elements[line]
#        
#        if "/*" in l:
#            del(elements[line])
#            continue
#            
#        elements[line] = l.split(" ")
#
#        print(elements[line])
#    
#    #@print(elements)
#    return elements
#        
#    print(data[line])   
    
    
if __name__ == "__main__":
    get_elements("c:\Temp\wlan_vdev.h", " _wlan_vdev ")
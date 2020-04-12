# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

#class Node:
#    def __init__(self, data, next=None):
#        self.data = data
#        self.next = next;
#    
#    def insert(self, data):
#        self.next=Node(data, self.next)
#        
#    def print(self):
#        node=self
#        while(node):
#            print(node.data)
#            node=node.next
#            
#
#node=Node(10)
#node.insert(20)
#node.insert(30)
#node.insert(40)

#node.print()



print ("******************* Tree Implementation  **************")
class Tree:
    def __init__(self, data, right=None, left=None):
        self.data = data
        self.right = right
        self.left = left
    
    def insert(self, data):
        if self.data > data:
            if self.left is None:
                self.left = Tree(data)
            else:
                self.left.insert(data)
        
        if self.data < data:
            if self.right is None:
                self.right = Tree(data)
            else:
                self.right.insert(data)
        
    def print(self):
        if self.left:
            self.left.print()
        print(self.data)
        if self.right:
            self.right.print()
            
    def isavailable(self, data):
        if self.data is data:
            return True
        if self.left:
            if self.left.isavailable(data):
                return True
        if self.right:
            if self.right.isavailable(data):
                return True
        return False
        
        
            

node=Tree(10)
node.insert(40)
node.insert(30)
node.insert(5)

node.print()
print (f"30 is available {node.isavailable(30)}")
print ("5 is availbe: %s" % node.isavailable(5))
print ("54 is availbe: {y}".format(y=node.isavailable(54)))
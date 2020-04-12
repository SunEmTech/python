# -*- coding: utf-8 -*-
"""
Created on Tue Sep  3 10:03:16 2019

@author: sandverm
"""

import requests
from bs4 import BeautifulSoup

title = input("Enter Movie name to search: ") 

s="+".join(title.split())


f_url = 'http://www.imdb.com/find?q='
url=f_url+s+'&s=all'
try:
	var = requests.get(url)
	soup = BeautifulSoup(var.content)

	x = soup.find("td", {"class": "result_text"})
	m = x.find("a")['href']

except Exception:
	print ("Check Your Movie Name")
	exit()
	
new_url = 'http://www.imdb.com' + m 

content = requests.get(new_url)
soup = BeautifulSoup(content.content)


x = soup.find("div", {"class": "title_wrapper"})
print ("-------------------------------------------------------------------")
c = x.findChildren()[0]
print ("Movie Name: %s" % c.text)

c = soup.find("div", {"class":"ratingValue"})
print ("IMDb: %s" % c.text) 



print ("--------------------------------------------------------------------")
print ("Director: ")
for tag in soup.find_all("span", {"itemprop":"director"}):
	print ("%s" % tag.text)

print ("--------------------------------------------------------------------")
print ("Writers: ")
for tag in soup.find_all("span", {"itemprop":"creator"}):
	print ("%s" % tag.text)


print ("--------------------------------------------------------------------")
print ("Actors: ")
for tag in soup.find_all("span", {"itemprop":"actors"}):
	print ("%s" % tag.text)


print ("--------------------------------------------------------------------")
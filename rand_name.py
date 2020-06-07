import random
import sys

num = sys.argv[1]

def generate_name(len=3):
    word = ""
    vowels = list('aeiou')
    v = ("a", "e", "i", "o", "u")
    key = list("tunnelconnectsandeepvermaaccessnetwotk")
    for i in range(len):
        c = chr(random.randint(97, 97+25))
        #c = random.choice(key)
        if i == len-2:
            c = random.choice(vowels)
        word +=c
    return word

names = set()
for i in range(100000):
    name = generate_name(int(num))
    print(name, end="  ")
    yes = input()
    if yes == "s":
        names.add(name)
    elif yes == "p":
        print("\n")
        for n in names:
            print(n, end="  ")
        print("\n")
        names.clear()


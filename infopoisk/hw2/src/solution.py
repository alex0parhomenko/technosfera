#!/usr/bin/python
import subprocess
import sys
import docreader
import numpy as np
import mmh3
from collections import Counter
from time import clock
import re
from time import sleep
from operator import itemgetter
import os

files = sys.argv

del files[0]
docs, urls = docreader.main(files)

hash_arr = []
hash_count = 20
shingle_k = 5
shingle = ""
SPLIT_RGX = re.compile(r'\w+', re.U)
for doc_id, doc in enumerate(docs):
    now_hash = [np.inf] * hash_count
    #print doc 
    #doc = doc.replace('\n', '')
    #print doc
    words = re.findall(SPLIT_RGX, doc)
    #words = doc.split(' ')
    #print words
    for i in range(len(words) - shingle_k + 1):
        shingle = ' '.join(words[i:i + shingle_k])
        #print shingle
        h = mmh3.hash(shingle)
        if (now_hash[h % hash_count] > h):
            now_hash[h % hash_count] = h
    for h in now_hash:
        if (h == np.inf):
            continue
        hash_arr.append([h, doc_id])

hash_arr = sorted(hash_arr)


now_pos = []
f = open("doc1_doc2.txt", "w")

for i in range(1, len(hash_arr)):
    if (hash_arr[i][0] == hash_arr[i - 1][0]):
        for p in now_pos:
            #f.write(str(hash_arr[p][1]) + " " + str(hash_arr[i][1]) + "\n")

            f.write('0' * (5 - len(str(hash_arr[p][1]))) + str(hash_arr[p][1]) + " " + '0' * (5 - len(str(hash_arr[i][1]))) + str(hash_arr[i][1]) + "\n")
        if (not now_pos):
            now_pos.append(i - 1)
            #f.write(str(hash_arr[i - 1][1]) + " " + str(hash_arr[i][1]) + '\n')

            f.write('0' * (5 - len(str(hash_arr[i - 1][1]))) + str(hash_arr[i - 1][1]) + " " + '0' * (5 - len(str(hash_arr[i][1]))) + str(hash_arr[i][1]) + '\n')

        now_pos.append(i)
    else:
        now_pos = []

os.system("sort --parallel=2 -no sort_doc.txt doc1_doc2.txt")
#exit(0)
hash_arr = []

cnt = 0
with open("sort_doc.txt") as ff:
    prev_f = ""
    prev_s = ""
    prev_line = ""
    cou = 0
    for line in ff:
        line = line.strip()
        if (line.find(' ') == -1):
            continue
        f, s = line.split(' ')
        if (f == prev_f and s == prev_s):
            cou += 1
        else:
            if (cou / (cou + 2.*(20. - cou)) > 0.75):
                now_f, now_s = prev_line.split(' ')
                now_f = int(now_f)
                now_s = int(now_s)
                cnt += 1
                print urls[now_f] + " " + urls[now_s] + " " + str(int(round(cou / (cou + 2.*(20. - cou)) * 100)))
            cou = 1
            prev_line = line
            prev_f = f
            prev_s = s

print cnt




#!/usr/bin/python
import subprocess
import sys
import docreader
import numpy as np
import mmh3
from collections import Counter
from time import clock

files = sys.argv
del files[0]
docs, urls = docreader.main(files)

hash_arr = []
hash_count = 20
shingle_k = 5
shingle = ""
hash_doc = Counter()
for doc_id, doc in enumerate(docs):
	now_hash = [np.inf] * hash_count 
	words = doc.split(' ')
	for i in range(len(words) - shingle_k):
		shingle = ' '.join(words[i:i + shingle_k])
		h = mmh3.hash(shingle)

		if (now_hash[h % hash_count] > h):
			now_hash[h % hash_count] = h
	for h in now_hash:
		if (h == np.inf):
			continue
		hash_arr.append([h, doc_id])

hash_arr = sorted(hash_arr)

now_pos = []
sim_arr = np.zeros((len(docs), len(docs)), dtype = np.float)

for i in range(1, len(hash_arr)):
	if (hash_arr[i][0] == hash_arr[i - 1][0]):
		for p in now_pos:
			sim_arr[hash_arr[i][1]][hash_arr[p][1]] += 1
			sim_arr[hash_arr[p][1]][hash_arr[i][1]] += 1
		if (not now_pos):
			now_pos.append(i - 1)
			now_pos.append(i)
			sim_arr[hash_arr[i][1]][hash_arr[i - 1][1]] += 1
			sim_arr[hash_arr[i - 1][1]][hash_arr[i][1]] += 1
		else:
			now_pos.append(i)
	else:
		now_pos = []

hash_arr = []
threshhold = 0.75

for doc_f in range(len(urls)):
	for doc_s in range(doc_f + 1, len(urls)):
		if (sim_arr[doc_f][doc_s] >= 15.0):
			print urls[doc_f] + " " + urls[doc_s] + " " + str(sim_arr[doc_f][doc_s] * 5)


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

pairs = []
now_pos = []
for i in range(1, len(hash_arr)):
	if (hash_arr[i][0] == hash_arr[i - 1][0]):
		for p in now_pos:
			pairs.append([min(hash_arr[i][1], hash_arr[p][1]), max(hash_arr[i][1], hash_arr[p][1])])
		if (not now_pos):
			now_pos.append(i - 1)
			now_pos.append(i)
			pairs.append([min(hash_arr[i][1], hash_arr[i - 1][1]), max(hash_arr[i][1], hash_arr[i - 1][1])])
		else:
			now_pos.append(i)
	else:
		now_pos = []

pairs = sorted(pairs)

result_pairs = Counter()
threshhold = 0.75
for pair_num, pair in enumerate(pairs):
	result_pairs[str(pair[0]) + " " + str(pair[1])] += 1


sim_id = []
for key in result_pairs:
	if (result_pairs[key] * 1.0 / hash_count >= threshhold):
		sim_id.append([result_pairs[key] * 1.0 / hash_count, key])
sim_id = sorted(sim_id, reverse = True)

for p in sim_id:
	first, second = p[1].split(' ')
  	print urls[int(first)] + " " + urls[int(second)] + " " + str(int(p[0] * 100))




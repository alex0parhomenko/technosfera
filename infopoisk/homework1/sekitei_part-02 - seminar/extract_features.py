import sys
import re
import random
from operator import itemgetter
import urlparse
import numpy as np
from urlparse import unquote
from collections import Counter


def extract_features(url_list, OUTPUT_FILE):
	f_out = open(OUTPUT_FILE, "w")

	result_url_list = url_list


	features_dict = Counter()
	substr = re.compile("{0}+\d+{0}+$".format('\D'))
	sub_ext = re.compile("{0}+\d+{0}+\..+$".format('\D'))
	ext = re.compile(".+\..+$")


	for i in range(len(result_url_list)):
		arr = result_url_list[i][2].split('/')
		arr = filter(None, arr)
		features_dict["segments:" + str(len(arr))] += 1

		d = urlparse.parse_qs(result_url_list[i][4])
		for key in d:
			features_dict["param_name:" + key] += 1
			features_dict["param:" + key + "=" + str(d[key])] += 1

		for j in range(len(arr)):
			features_dict["segment_name_" + str(j) + ":" + str(arr[j])] += 1
			features_dict["segment_len_" + str(j) +  ":" + str(len(arr[j]))] += 1

			if (arr[j].isdigit()):
				features_dict["segment_[0-9]_" + str(j) + ":1"] += 1

			if (re.match(substr, arr[j]) is not None):
				features_dict["segment_substr[0-9]_" + str(j) + ":1"] += 1
			
			if (re.match(ext, arr[j]) is not None):
				features_dict["segment_ext_" + str(j) + ":" + arr[j][arr[j].rfind('.') + 1:]] += 1
			
			if (re.match(sub_ext, arr[j]) is not None):
				features_dict["segment_ext_substr[0-9]_" + str(j) + ":" + arr[j][arr[j].rfind('.') + 1:]] += 1

	result_features_list = []

	for key in features_dict:
		if (features_dict[key] < 40):
			continue
		result_features_list.append([features_dict[key], key])

	result_features_list = sorted(result_features_list, reverse = True)
	for i in range(len(result_features_list)):
		f_out.write(str(result_features_list[i][1]) + "\t" + str(result_features_list[i][0]) + "\n")


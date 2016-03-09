import sys
import re
import random
from operator import itemgetter
import urlparse
import numpy as np
from urlparse import unquote
from sklearn.feature_extraction import DictVectorizer
from collections import Counter

substr = re.compile("{0}+\d+{0}+$".format('\D'))
sub_ext = re.compile("{0}+\d+{0}+\..+$".format('\D'))
ext = re.compile(".+\..+$")

def extract_features(url_list):
	result_url_list = url_list
	list_dict = []
	global substr
	global sub_ext
	global ext	

	for i in xrange(len(result_url_list)):
		features_dict = Counter()

		arr = result_url_list[i][2].split('/')
		arr = filter(None, arr)
		features_dict["segments:" + str(len(arr))] += 1

		d = urlparse.parse_qs(result_url_list[i][4])
		for key in d:
			features_dict["param_name:" + key] += 1
			features_dict["param:" + key + "=" + str(d[key])] += 1

		for j in xrange(len(arr)):
			features_dict["segment_name_" + str(j) + ":" + str(arr[j])] += 1
			#features_dict["segment_len_" + str(j) +  ":" + str(len(arr[j]))] += 1

			if (arr[j].isdigit()):
				features_dict["segment_[0-9]_" + str(j) + ":1"] += 1

			if (re.match(substr, arr[j]) is not None):
				features_dict["segment_substr[0-9]_" + str(j) + ":1"] += 1
			
			if (re.match(ext, arr[j]) is not None):
				features_dict["segment_ext_" + str(j) + ":" + arr[j][arr[j].rfind('.') + 1:]] += 1
			
			if (re.match(sub_ext, arr[j]) is not None):
				features_dict["segment_ext_substr[0-9]_" + str(j) + ":" + arr[j][arr[j].rfind('.') + 1:]] += 1
		list_dict.append(features_dict)

	return list_dict





import sys
import re
import random
from operator import itemgetter
import urlparse
import numpy as np

def extract_features(INPUT_FILE_1, INPUT_FILE_2, OUTPUT_FILE):

	f_in1 = open(INPUT_FILE_1, "r")
	f_in2 = open(INPUT_FILE_2, "r")
	f_out = open(OUTPUT_FILE, "w")
	url_list1 = []
	url_list2 = []
	result_url_list = []

	for num, link in enumerate(f_in1):
		link = link.strip("\n")
		link = link.strip("/")
		link_arr = urlparse.urlparse(link).path.split("/")[1:len(link)]
		url_list1.append(link_arr)
		
	for num, link in enumerate(f_in2):
		link = link.strip("\n")
		link = link.strip("/")
		link_arr = urlparse.urlparse(link).path.split("/")[1:len(link)]
		url_list2.append(link_arr)

	url_list1 = np.asarray(url_list1)
	url_list2 = np.asarray(url_list2)
	np.random.shuffle(url_list1)
	np.random.shuffle(url_list2)
	result_url_list = np.concatenate((url_list1[0:1000], url_list2[0:1000]), axis = 0)

		
	segments = {}
	len_segments = {}
	match_with_string = {}
	consist_of_numbers = {}
	str_num_str = {}
	expansion = {}
	expansion_and_str_num_str = {}
	param_name = {}
	param = {}


	substr1 = re.compile(r'^[a-zA-Z]+[0-9]+[a-zA-Z]*$')
	substr2 = re.compile(r'^[a-zA-Z]*[0-9]+[a-zA-Z]+$')

	for i in range(len(result_url_list)):
		segments.update({"segments:" + str(len(result_url_list[i])): segments.get("segments:" + str(len(result_url_list[i])), 0) + 1})

		for j in range(len(result_url_list[i])):


			d = urlparse.parse_qs(urlparse.urlparse(result_url_list[i][j]).query)
			for key in d:
				param_name.update({"param_name:" + key: param_name.get("param_name:" + key, 0) + 1})
				param.update({"param:" + key + "=" + str(d[key]) : param.get("param:" + key + "=" + str(d[key]), 0) + 1})

			match_with_string.update({"segment_name_" + str(j) + ":" + str(result_url_list[i][j]): match_with_string.get("segment_name_" + str(j) + ":" + str(result_url_list[i][j]), 0) + 1})

			if (result_url_list[i][j].isdigit()):
				consist_of_numbers.update({"segment_[0-9]_" + str(j) + ":1": consist_of_numbers.get("segment_[0-9]_" + str(j) + ":1", 0) + 1})

			if (len(substr1.findall(result_url_list[i][j])) + len(substr2.findall(result_url_list[i][j])) > 0):
				str_num_str.update({"segment_substr[0-9]_" + str(j) + ":1": str_num_str.get("segment_substr[0-9]_" + str(j) + ":1", 0) + 1})

			s = result_url_list[i][j].rsplit('.', 1)
			if (len(s) > 1 and len(s[1]) != 0):
				if ((len(substr1.findall(s[0])) + len(substr2.findall(s[0])) > 0)):
					expansion_and_str_num_str.update({"segment_ext_substr[0-9]_" + str(j) + ":" + s[1]: expansion_and_str_num_str.get("segment_ext_substr[0-9]_" + str(j) + ":" + s[1], 0) + 1})
				expansion.update({"segment_ext_" + str(j) + ":" + s[1]: expansion.get("segment_ext_" + str(j) + ":" + s[1], 0) + 1})
		
			len_segments.update({"segment_len_" + str(j) +  ":" + str(len(result_url_list[i][j])): len_segments.get("segment_len_" + str(j) +  ":" + str(len(result_url_list[i][j])), 0) + 1})
	result_features_list = []
	for key in segments:
		result_features_list.append([segments[key], key])
	for key in len_segments:
		result_features_list.append([len_segments[key], key])
	for key in match_with_string:
		result_features_list.append([match_with_string[key], key])
	for key in consist_of_numbers:
		result_features_list.append([consist_of_numbers[key], key])
	for key in str_num_str:
		result_features_list.append([str_num_str[key], key])
	for key in expansion:
		result_features_list.append([expansion[key], key])
	for key in expansion_and_str_num_str:
		result_features_list.append([expansion_and_str_num_str[key], key])
	for key in param_name:
		result_features_list.append([param_name[key], key])
	for key in param:
		result_features_list.append([param[key], key])
	result_features_list = sorted(result_features_list, reverse = True)
	for i in range(len(result_features_list)):
		if (result_features_list[i][0] <= 100):
			break
		f_out.write(str(result_features_list[i][1]) + "\t" + str(result_features_list[i][0]) + "\n")



extract_features("data/urls.lenta.examined", "data/urls.lenta.general", "output.txt")


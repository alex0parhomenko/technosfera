import sys
import os
import re
import random
import time
import numpy as np
import sklearn as sk
from urlparse import unquote
from sklearn.cluster import AgglomerativeClustering
import urlparse
from sklearn.feature_extraction import DictVectorizer
from sklearn.decomposition import PCA
from sklearn.decomposition import FastICA
from sklearn.cluster import KMeans
from sklearn.cluster import Birch
from sklearn.metrics import silhouette_score
from sklearn.feature_selection import SelectKBest
from sklearn.feature_selection import chi2
from sklearn.feature_selection import f_classif
from collections import Counter


sekitei = None;
brc = None
quota = 10000
select = None
quota_for_each_cluster = np.zeros(1)
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


def out_data(output, data, start_url):
	f = open(output, "w")
	f.write("$TYPE vec_bin\n")
	f.write("$XDIM " + str(len(data)) + "\n")
	f.write("$YDIM 1\n")
	f.write("$VEC_DIM " + str(data.shape[1]) + "\n")

	for i in xrange(data.shape[0]):
		for j in xrange(data.shape[1]):
			f.write(str(data[i][j]) + " ")
		f.write(str(start_url[i]) + "\n")

def out_template(output, features, data_len):
	f = open(output, "w")
	f.write("$TYPE template\n")
	f.write("$XDIM 2\n")
	f.write("$YDIM " + str(data_len) + "\n")
	f.write("$VEC_DIM " + str(len(features)) + "\n")
	for i in range(len(features)):
		f.write(str(i) + " " + str(features[i]) + "\n")

def define_segments(QLINK_URLS, UNKNOWN_URLS, QUOTA):
	#t = time.clock()
	global quota_for_each_cluster
	global brc
	global v
	global quota
	global select
	quota = 10000
	result_arr = QLINK_URLS + UNKNOWN_URLS
	for i, url in enumerate(result_arr):
		result_arr[i] = urlparse.urlparse(unquote(url.strip()))

	#l_dict = 
	v = DictVectorizer(sparse=False)
	data = v.fit_transform(extract_features(result_arr))
	ind_list = []
	ind_list_data = []
	low_bound = 8

	for col in xrange(data.shape[1]):
		if (np.sum(data[:, col]) > low_bound):
			ind_list.append(1)
			ind_list_data.append(col)
		else:
			ind_list.append(0)

	v = v.restrict(ind_list)
	data = data[:, ind_list_data] 
	#if (start_url[0].find("wikipedia") != -1):
	#	out_data("som_data_wiki/qlink.tfxidf", data[:500], start_url[:500])
	#	out_data("som_data_wiki/notqlink.tfxidf", data[500:], start_url[500:])
	#	out_data("som_data_wiki/data.tfxidf", data, start_url)
	#	out_template("som_data_wiki/data_features.tv", v.get_feature_names(), len(data))
	#	out_template("som_data_wiki/qlink_features.tv", v.get_feature_names(), len(data) / 2)
	#	out_template("som_data_wiki/notqlink_features.tv", v.get_feature_names(), len(data) / 2)
	#	return 0
	best_cou_clusters = data.shape[1]
	#k_means = KMeans(n_clusters=best_cou_clusters, init = 'random')
	#clust = k_means.fit_predict(data)
	brc = Birch(branching_factor=50, n_clusters=best_cou_clusters, threshold=0.2, compute_labels=True)
	clust = brc.fit_predict(data)
	select = SelectKBest(k=min(data.shape[1], 30))
	data = select.fit_transform(data, clust)
	clust = brc.fit_predict(data)
	#print data.shape

	quota_for_each_cluster = np.zeros(best_cou_clusters)
	clust_qlink = list(clust[:500])
	for i in xrange(best_cou_clusters):
		quota_for_each_cluster[i] = clust_qlink.count(i) / 500.0 * QUOTA 
	quota_for_each_cluster *= 2.0
	#print quota_for_each_cluster
	#print time.clock() - t



def fetch_url(url):
	global quota_for_each_cluster
	global brc
	global v
	global select
	global quota
	if (quota == 0):
		return False
	
	#data = select.transform(v.transform(extract_features([urlparse.urlparse(unquote(url.strip()))])))
	#print data.shape
	nearest_clust = (brc.predict(select.transform(v.transform(extract_features([urlparse.urlparse(unquote(url.strip()))])))))[0]
	#print nearest_clust
	if (quota_for_each_cluster[nearest_clust] > 0):
		quota_for_each_cluster[nearest_clust] -= 1
		quota -= 1
		return True
	else:
		return False


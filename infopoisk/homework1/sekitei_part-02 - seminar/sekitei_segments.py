import sys
import os
import re
import random
import time
import numpy as np
import sklearn as sk
import extract_features
from urlparse import unquote
from sklearn.cluster import AgglomerativeClustering
import urlparse
from sklearn.decomposition import PCA
from sklearn.decomposition import FastICA
from sklearn.cluster import KMeans
from sklearn.metrics import silhouette_score
import sompy as SOM



sekitei = None;
cluster_centroids = np.zeros(1)
quota_for_each_cluster = np.zeros(1)
pca = None

def define_segments(QLINK_URLS, UNKNOWN_URLS, QUOTA):
	global pca
	global cluster_centroids
	global quota_for_each_cluster
	result_arr = QLINK_URLS + UNKNOWN_URLS
	for i in range(len(result_arr)):
		result_arr[i] = urlparse.urlparse(unquote(result_arr[i].strip()))

	extract_features.extract_features(result_arr, "top_features.txt")

	sub_str = re.compile("{0}+\d+{0}+$".format('\D'))
	sub_ext = re.compile("{0}+\d+{0}+\..+$".format('\D'))
	ext = re.compile(".+\..+$")

	f = open("top_features.txt", "r")
	real_features = []
	for line in f:
		real_features.append(((line.strip()).split("\t"))[0])

	str0 = "segments:"
	str1 = "param_name:"
	str2 = "param:"
	str3 = "segment_name_"
	str4 = "segment_[0-9]_"
	str5 = "segment_substr[0-9]_"
	str6 = "segment_ext_"
	str7 = "segment_ ext_substr[0-9]_"
	str8 = "segment_len_"
	lenght_arr = [len(str0), len(str1), len(str2), len(str3), len(str4), len(str5), len(str6), len(str7), len(str8)]

	data = np.zeros(shape = (len(result_arr), len(real_features)))

	for i in xrange(len(result_arr)):
		arr = result_arr[i][2].split('/')
		arr = filter(None, arr)

		d = urlparse.parse_qs(result_arr[i][4])

		for num_feature, feature in enumerate(real_features):
		
			if (feature.find(str0, 0, lenght_arr[0]) != -1):
				if (len(arr) == (int)(feature[lenght_arr[0]: ])):
					data[i][num_feature] = 1

			elif (feature.find(str1, 0, lenght_arr[1]) != -1):
				for key in d:
					if (key == feature[lenght_arr[1]: ]):
							data[i][num_feature] = 1
							break

			elif (feature.find(str2, 0, lenght_arr[2]) != -1):
				for key in d:
					if ((key + "=" + d[key]) == feature[lenght_arr[2]:]):
						data[i][num_feature] = 1
						break


			elif (feature.find(str3, 0, lenght_arr[3]) != -1 and  (int)(feature[lenght_arr[3]:feature.find(':')]) < len(arr) and arr[(int)(feature[lenght_arr[3]:feature.find(':')])] == feature[feature.find(':') + 1:]):
				data[i][num_feature] = 1


			elif (feature.find(str4, 0, lenght_arr[4]) != -1 and (int)(feature[lenght_arr[4]:feature.find(':')]) < len(arr) and (arr[(int)(feature[lenght_arr[4]:feature.find(':')])]).isdigit()):
				data[i][num_feature] = 1

			elif (feature.find(str5, 0, lenght_arr[5]) != -1 and (int)(feature[lenght_arr[5]:feature.find(':')]) < len(arr) and re.match(sub_str, arr[(int)(feature[lenght_arr[5]:feature.find(':')])]) is not None ):
				data[i][num_feature] = 1

			elif (feature.find(str6, 0, lenght_arr[6]) != -1 and (int)(feature[lenght_arr[6]:feature.find(':')]) < len(arr) and arr[(int)(feature[lenght_arr[6]:feature.find(':')])].find("." + feature[feature.find(":") +1:]) != -1):
				data[i][num_feature] = 1

			elif (feature.find(str7, 0, lenght_arr[7]) != -1 and (int)(feature[lenght_arr[7]:feature.find(':')]) < len(arr) and re.match(sub_ext, arr[(int)(feature[lenght_arr[7]:feature.find(':')])]) is not None and (arr[(int)(feature[lenght_arr[7]:feature.find(':')])]).find(feature[feature.find(":") + 1:]) != -1):
				data[i][num_feature] = 1

			elif (feature.find(str8, 0, lenght_arr[8]) != -1 and (int)(feature[lenght_arr[8]:feature.find(':')]) < len(arr) and len(arr[(int)(feature[lenght_arr[8]:feature.find(':')])]) == int(feature[feature.find(':') + 1:])):
				data[i][num_feature] = 1 
	#clust = AgglomerativeClustering(linkage = "average", n_clusters = 4, affinity = "precomputed")
	#print "all good"
	#pca = FastICA(n_components=3)
	#pca.fit(data)
	#new_data = pca.transform(data)
	new_data = data
	#f = open("data.txt", "w")
	#for a in new_data:
	#	f.write(str(a[0]) + " " + str(a[	1]) + "\n")
	#print new_data
	max_cou_clusters = 6
	best_cou_clusters = None
	best_score = None
	for cou_clusters in xrange(2, max_cou_clusters):
		clust = KMeans(n_clusters=cou_clusters, init = 'k-means++').fit_predict(new_data)
		#clust = list(clust)
		sc = silhouette_score(new_data, clust)
		best_cou_clusters = cou_clusters if sc > best_score else best_cou_clusters
		best_score = sc if sc > best_score else best_score

	clust = KMeans(n_clusters=best_cou_clusters, init = 'k-means++').fit_predict(new_data)

	cou_samples_in_each_cluster = np.zeros((best_cou_clusters, new_data.shape[1]))
	cluster_centroids = np.zeros((best_cou_clusters, new_data.shape[1]))

	for pos, clust_num in enumerate(clust):
		cou_samples_in_each_cluster[clust_num] += np.tile(1, new_data.shape[1])
		cluster_centroids[clust_num] += new_data[pos]


	cluster_centroids = cluster_centroids / cou_samples_in_each_cluster
	#print cluster_centroids
	quota_for_each_cluster = np.zeros(best_cou_clusters)
	clust_qlink = list(clust[:500])
	for i in xrange(best_cou_clusters):
		quota_for_each_cluster[i] = clust_qlink.count(i) * 1. / len(clust_qlink) * QUOTA 
	#print quota_for_each_cluster 
	#print clust.count(1), clust.count(2), clust.count(0), clust.count(3), clust.count(4)



def fetch_url(url):
	global cluster_centroids
	global quota_for_each_cluster
	f = open("top_features.txt", "r")
	real_features = []
	for line in f:
		real_features.append(((line.strip()).split("\t"))[0])

	result_arr = []
	result_arr.append(url)
	result_arr[0] = urlparse.urlparse(unquote(result_arr[0].strip()))
	str0 = "segments:"
	str1 = "param_name:"
	str2 = "param:"
	str3 = "segment_name_"
	str4 = "segment_[0-9]_"
	str5 = "segment_substr[0-9]_"
	str6 = "segment_ext_"
	str7 = "segment_ ext_substr[0-9]_"
	str8 = "segment_len_"
	lenght_arr = [len(str0), len(str1), len(str2), len(str3), len(str4), len(str5), len(str6), len(str7), len(str8)]

	data = np.zeros(shape = (len(result_arr), len(real_features)))

	for i in xrange(len(result_arr)):
		arr = result_arr[i][2].split('/')
		arr = filter(None, arr)

		d = urlparse.parse_qs(result_arr[i][4])

		for num_feature, feature in enumerate(real_features):
		
			if (feature.find(str0, 0, lenght_arr[0]) != -1):
				if (len(arr) == (int)(feature[lenght_arr[0]: ])):
					data[i][num_feature] = 1

			elif (feature.find(str1, 0, lenght_arr[1]) != -1):
				for key in d:
					if (key == feature[lenght_arr[1]: ]):
							data[i][num_feature] = 1
							break

			elif (feature.find(str2, 0, lenght_arr[2]) != -1):
				for key in d:
					if ((key + "=" + d[key]) == feature[lenght_arr[2]:]):
						data[i][num_feature] = 1
						break


			elif (feature.find(str3, 0, lenght_arr[3]) != -1 and  (int)(feature[lenght_arr[3]:feature.find(':')]) < len(arr) and arr[(int)(feature[lenght_arr[3]:feature.find(':')])] == feature[feature.find(':') + 1:]):
				data[i][num_feature] = 1


			elif (feature.find(str4, 0, lenght_arr[4]) != -1 and (int)(feature[lenght_arr[4]:feature.find(':')]) < len(arr) and (arr[(int)(feature[lenght_arr[4]:feature.find(':')])]).isdigit()):
				data[i][num_feature] = 1

			elif (feature.find(str5, 0, lenght_arr[5]) != -1 and (int)(feature[lenght_arr[5]:feature.find(':')]) < len(arr) and re.match(sub_str, arr[(int)(feature[lenght_arr[5]:feature.find(':')])]) is not None ):
				data[i][num_feature] = 1

			elif (feature.find(str6, 0, lenght_arr[6]) != -1 and (int)(feature[lenght_arr[6]:feature.find(':')]) < len(arr) and arr[(int)(feature[lenght_arr[6]:feature.find(':')])].find("." + feature[feature.find(":") +1:]) != -1):
				data[i][num_feature] = 1

			elif (feature.find(str7, 0, lenght_arr[7]) != -1 and (int)(feature[lenght_arr[7]:feature.find(':')]) < len(arr) and re.match(sub_ext, arr[(int)(feature[lenght_arr[7]:feature.find(':')])]) is not None and (arr[(int)(feature[lenght_arr[7]:feature.find(':')])]).find(feature[feature.find(":") + 1:]) != -1):
				data[i][num_feature] = 1

			elif (feature.find(str8, 0, lenght_arr[8]) != -1 and (int)(feature[lenght_arr[8]:feature.find(':')]) < len(arr) and len(arr[(int)(feature[lenght_arr[8]:feature.find(':')])]) == int(feature[feature.find(':') + 1:])):
				data[i][num_feature] = 1 

	vect = data[0]
	nearest_dist = 10000
	nearest_clust = None
	#vect = pca.transform(vect)
	#cluster_centroids

	for i in xrange(len(cluster_centroids)):
		dist = np.sqrt(np.sum((cluster_centroids[i] - vect) ** 2))
		nearest_clust = i if dist < nearest_dist else nearest_clust
		nearest_dist = dist if dist < nearest_dist else nearest_dist

	if (quota_for_each_cluster[nearest_clust] > 0):
		quota_for_each_cluster[nearest_clust] -= 1
		return True
	else:
		return False


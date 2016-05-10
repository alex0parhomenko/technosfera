# coding: utf-8
import sys
import numpy  as np
import random
import base64
from html_parser import SpamHTMLParser
from sklearn.externals import joblib
from sklearn.metrics import f1_score
from sklearn import cross_validation
from sklearn.ensemble import GradientBoostingClassifier 
from nltk.corpus import stopwords
from sklearn.cross_validation import KFold
from nltk.stem import SnowballStemmer
from sklearn.feature_extraction import DictVectorizer
from collections import Counter                
import pickle
from nltk import FreqDist
from zlib import compress
import string

clf = joblib.load('model/model.pkl')
stemmer = SnowballStemmer("russian");
table = string.maketrans("","")

features_list = []

with open('words') as f:
    for num, line in enumerate(f):
        if num == 1000:
            break
        line = line.strip()
        f, s = line.split(' ')
        features_list.append(f)

features_list = list(set(features_list))

def test_trans(s):
    return s.translate(table, string.punctuation)

class StatsCollector:
    def __init__(self):
        self.total_words = None;
        self.header_words = None;
        self.average_word_length = None;
        self.links_words = None;
        self.mark = None;
        self.compress = None;
        self.word_features = []
        self.strong_words = None
        self.url_words = None
        self.url_digits = None
        self.em_words = None
        self.word_features_in_tag = None
        
    def collect(self, mark, pageInb64, url):
        global features_list

        html = base64.b64decode(pageInb64).decode('utf-8')
        parser = SpamHTMLParser()
        parser.feed(html)

        text = test_trans(parser.text().encode('utf-8')).replace('  ', ' ')
        titletext = test_trans(parser.titletext().encode('utf-8')).replace('  ', ' ')
        atext = test_trans(parser.atext().encode('utf-8')).replace('  ', ' ')

        strongtext = test_trans(parser.strongtext().encode('utf-8')).replace('  ', ' ')
        emtext = test_trans(parser.emtext().encode('utf-8')).replace('  ', ' ')

        doc_len = len(text.split(' '))


        arr = np.asarray(map(lambda x: len(x), parser.text().split(' ')), dtype = np.int)
        self.compress = len(compress(text)) * 1. / len(text)
        self.average_word_length = int(np.sum(arr) * 1. / arr.shape[0])
        self.total_words = text.count(' ') 
        self.header_words = titletext.count(' ')
        self.links_words = atext.count(' ')
        self.strong_words = strongtext.count(' ')
        self.em_words = emtext.count(' ')
        self.url_words = url.count('.') + url.count('/') + url.count('_') + url.count('-')
        self.url_digits = 0

        for i in range(10):
            self.url_digits += url.count(str(i))
        
        #l = np.asarray(map(lambda x: text.count(x), features_list))
        #l /= doc_len
        #term_frequency.append(l)
        #l = np.asarray(map(lambda x: bool(text.count(x)), features_list))
        #document_frequency += l
        self.mark = mark
        tag_text = emtext + ' ' + strongtext + ' ' + atext
        #print tag_text, len(text), len(tag_text), "\n\n"
        self.word_features = map(lambda w: text.count(w), features_list)
        self.word_features_in_tag = map(lambda w: tag_text.count(w), features_list)
        parser.close()

    def get_features(self):
        return [self.total_words, self.header_words, self.average_word_length, self.links_words,\
         self.compress, self.strong_words, self.em_words, self.url_words, self.url_digits] \
          + self.word_features_in_tag + self.word_features

    def get_target(self):
        return self.mark
            


def fit_model():
    DATA_FILE  = './data/train-set-ru-b64-utf-8.txt'
    stats_collector = StatsCollector()
    i=0
    data = []
    target = []

    with open (DATA_FILE) as df:
         for i, line in enumerate(df):
            print i
            line = line.strip()
            parts = line.split()
            stats_collector = StatsCollector()
            stats_collector.collect(int(parts[1]), parts[3], parts[2])
            data.append(stats_collector.get_features())
            target.append(stats_collector.get_target())
            #print len(data[-1])


    data = np.asarray(data, dtype = np.float)
    target = np.asarray(target, dtype = np.float)
    print data.shape, target.shape
    df.close()
    clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.07, n_estimators=300, min_samples_split=30,\
         min_samples_leaf=15, max_depth=4)

    clf.fit(data, target)
    y_pred = clf.predict(data)
    print f1_score(target, y_pred)

    joblib.dump(clf, 'model/model.pkl') 


def cv_model():
    DATA_FILE  = './data/train-set-ru-b64-utf-8.txt'
    all_data = []
    target = []
    with open(DATA_FILE) as df:
        for i, line in enumerate(df):
            print i
            line = line.strip()
            parts = line.split()
            stats_collector = StatsCollector()
            #print parts[2]
            #print base64.b64decode(parts[3])#.decode('utf-8')
            #print parts[2].decode('utf-8'), parts[3].decode('utf-8'), "\n"
            stats_collector.collect(int(parts[1]), parts[3], parts[2])
            # mark page url
            all_data.append(stats_collector.get_features())
            target.append(stats_collector.get_target())
            #print all_data[-1]

    data = np.asarray(all_data, dtype = np.float)
    target = np.asarray(target, dtype = np.float)

    clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.05, n_estimators=400,\
     min_samples_split=30, min_samples_leaf=15, max_depth=5)

    kf = KFold(data.shape[0], n_folds = 3, shuffle = True)

    for train_index, test_index in kf:
        X_train, X_test = data[train_index], data[test_index]
        y_train, y_test = target[train_index], target[test_index]
        clf.fit(X_train, y_train)
        y_pred = clf.predict(X_test)
        print f1_score(y_test, y_pred)



def is_spam(pageInb64, url):
    global features_list
    global clf

    original = base64.b64decode(pageInb64)
    # check url  
    stats_collector = StatsCollector()
    stats_collector.collect(None, pageInb64, url)
    features = np.asarray(stats_collector.get_features(), dtype = np.float)
    target = clf.predict_proba(features.reshape(1, -1))
    if (target[0][1] > 0.9):
        return 1
    else:
        return 0


def main():
    global features_list

    fit_model()
    return 0

if __name__ == '__main__':
    main()
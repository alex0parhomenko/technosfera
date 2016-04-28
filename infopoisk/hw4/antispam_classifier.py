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

features_list = None
document_frequency = None
term_frequency = None
doc_count = None


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
        
    def collect(self, mark, pageInb64, url):
        global features_list
        global document_frequency
        global term_frequency
        global doc_count

        doc_count += 1
        html = base64.b64decode(pageInb64).decode('utf-8')
        parser = SpamHTMLParser()
        parser.feed(html)

        text = test_trans(parser.text().encode('utf-8'))
        titletext = test_trans(parser.titletext().encode('utf-8'))
        atext = test_trans(parser.atext().encode('utf-8'))
        strongtext = test_trans(parser.strongtext().encode('utf-8'))
        emtext = test_trans(parser.emtext().encode('utf-8'))

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

        self.word_features = map(lambda w: text.count(w), features_list)
        parser.close()

    def get_features(self):
        return [self.total_words, self.header_words, self.average_word_length, self.links_words,\
         self.compress, self.strong_words, self.em_words, self.url_words, self.url_digits] + self.word_features

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
                line = line.strip()
                parts = line.split()
                stats_collector = StatsCollector()
                stats_collector.collect(int(parts[1]), parts[3], parts[2])
                data.append(stats_collector.get_features())
                target.append(stats_collector.get_target())

    data = np.asarray(data, dtype = np.float)
    target = np.asarray(target, dtype = np.float)
    print data.shape, target.shape
    df.close()
    clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.07, n_estimators=330, min_samples_split=30,\
         min_samples_leaf=1, max_depth=4)

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
            stats_collector.collect(int(parts[1]), parts[3], parts[2])
            # mark page url
            all_data.append(stats_collector.get_features())
            target.append(stats_collector.get_target())
            #print all_data[-1]

    data = np.asarray(all_data, dtype = np.float)
    target = np.asarray(target, dtype = np.float)

    clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.07, n_estimators=400,\
     min_samples_split=30, min_samples_leaf=1, max_depth=4)

    kf = KFold(data.shape[0], n_folds = 3, shuffle = True)

    for train_index, test_index in kf:
        X_train, X_test = data[train_index], data[test_index]
        y_train, y_test = target[train_index], target[test_index]
        clf.fit(X_train, y_train)
        y_pred = clf.predict(X_test)
        print f1_score(y_test, y_pred)

def check_naive_bayes():
    global term_frequency
    global document_frequency
    global doc_count
    DATA_FILE  = './data/train-set-ru-b64-utf-8.txt'
    all_data = []
    target = []

    with open(DATA_FILE) as df:
        for i, line in enumerate(df):
            print i
            line = line.strip()
            parts = line.split()
            stats_collector = StatsCollector()
            stats_collector.collect(int(parts[1]), parts[3], parts[2])

            all_data.append(stats_collector.get_features())
            target.append(stats_collector.get_target())

    data = np.asarray(all_data, dtype = np.float)
    target = np.asarray(target, dtype = np.float)

    document_frequency = np.log(doc_count / df)
    term_frequency = np.asarray(tf, dtype = np.float)
    document_frequency = np.asarray(df, dtype = np.float)
    tf_df_matrix = np.dot(tf, df.T)

    print tf_df_matrix.shape



def is_spam(pageInb64, url):
    global features_list
    global clf

    features_list = []
    open_corpora_dict = []
    with open('words') as f:
        for num, line in enumerate(f):
            if num == 500:
                break
            line = line.strip()
            f, s = line.split(' ')
            features_list.append(f)

    original = base64.b64decode(pageInb64)
    # check url  
    stats_collector = StatsCollector()
    stats_collector.collect(None, pageInb64, url)
    features = np.asarray(stats_collector.get_features(), dtype = np.float)
    target = clf.predict(features.reshape(1, -1))
    return target


def main():
    global features_list
    global D
    global term_frequency
    global document_frequency
    global doc_count
    D = {}
    features_list = []

    with open('words') as f:
        for num, line in enumerate(f):
            if (num == 20000):
                break
            line = line.strip()
            f, s = line.split(' ')
            features_list.append(f)


    document_frequency = np.zeros(len(features_list))
    term_frequency = []
    doc_count = 0
    check_naive_bayes()
    return 0
    fit_model()
    return 0

if __name__ == '__main__':
    main()
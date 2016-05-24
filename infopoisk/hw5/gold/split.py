# -*- coding: utf-8 -*-
import re
import sys, json
from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.externals import joblib
import numpy as np
from sklearn.metrics import accuracy_score
from sklearn.linear_model import LogisticRegression
from sklearn.cross_validation import KFold
import xgboost.sklearn as xgb

def findOccurences(s, ch1, ch2, ch3):
    return [i for i, letter in enumerate(s) if letter == ch1 or letter == ch2 or letter == ch3]

def remove_spaces(s):
    pos = 0
    while s[pos] == ' ':
        pos += 1
    return s[pos:]

class extract_features:
    def __init__(self):
        self.end_type = []
        self.is_next_letter_up = []
        self.is_next_space = []
        self.len_prev_word = []
        self.count_words_to_prev_point = []
        self.count_words_to_prev_end_symbol = []
        self.is_prev_word_start_up_letter = []
        self.is_prev_letter_point = []
        self.is_next_letter_point = []
        self.is_last_symbol = []
        self.is_prev_letter_digit = []
        self.mark = []

    def collect_features(self, par, sen = None):
        par = par.strip()
        ind_par = findOccurences(par, '.', '?', '!')
        ind_true = []
        pos = 0
        if sen != None:
            for num, s in enumerate(sen):
                pos += len(s) 
                if num == 0:
                    ind_true.append(pos - 1)
                else:
                    ind_true.append(pos)
                    pos += 1

            for ind in ind_par:
                self.mark.append(1 if ind in ind_true else 0)


        prev_ind = 0
        for ind in ind_par:
            if par[ind] == '.':
                self.end_type.append([0, 0, 1])
            elif par[ind] == '!':
                self.end_type.append([1, 0, 0])
            elif par[ind] == '?':
                self.end_type.append([0, 1, 0])

            if ind + 1 < len(par) and par[ind + 1].isupper():
                self.is_next_letter_up.append(1)
            else:
                self.is_next_letter_up.append(0)

            if ind + 1 < len(par) and par[ind + 1] == ' ':
                self.is_next_space.append(1)
            else:
                self.is_next_space.append(0)

            if ind + 1 < len(par) and par[ind + 1] == '.':
                self.is_next_letter_point.append(1)
            else:
                self.is_next_letter_point.append(0)

            if ind + 1 == len(par):
                self.is_last_symbol.append(1)
            else:
                self.is_last_symbol.append(0)

            p = par.rfind(' ', 0, ind)
            if (p == -1):
                self.len_prev_word.append(ind)
            else:
                self.len_prev_word.append(ind - p - 1)

            if par[p].isupper():
                self.is_prev_word_start_up_letter.append(1)
            else:
                self.is_prev_word_start_up_letter.append(0)

            self.count_words_to_prev_point.append(len(filter(lambda x: len(x) > 0, par[prev_ind + 1:ind].split(' '))))

            prev_end_symbol = max(par.rfind('!', 0, ind), par.rfind('?', 0, ind), par.rfind('.', 0, ind), 0)
            #print prev_end_symbol, ind
            self.count_words_to_prev_end_symbol.append(len(filter(lambda x: len(x) > 0, par[prev_end_symbol + 1:ind].split(' '))))

            if ind - 1 >= 0 and par[ind - 1] == '.':
                self.is_prev_letter_point.append(1)
            else:
                self.is_prev_letter_point.append(0)

            if ind - 1 >= 0 and par[ind - 1].isdigit():
                self.is_prev_letter_digit.append(1)
            else:
                self.is_prev_letter_digit.append(0)

            prev_ind = ind

    def pick_features(self):
        return [self.is_next_letter_up, self.is_next_space, self.len_prev_word, \
        self.count_words_to_prev_point, self.is_prev_word_start_up_letter, \
        self.is_prev_letter_point, self.is_last_symbol, self.count_words_to_prev_end_symbol,\
         self.is_prev_letter_digit, self.is_next_letter_point]

    def get_target(self):
        return self.mark

    def get_features(self):
        end_type = np.asarray(self.end_type, dtype = np.int)
        other_features = np.asarray(self.pick_features(), dtype = np.float).T
        return np.concatenate((end_type, other_features), axis = 1)
            
        


def fit_model(data, target):
    clf = xgb.XGBClassifier(max_depth = 3, n_estimators = 150, learning_rate = 0.07, reg_alpha = 0.7, reg_lambda = 0.7)
    #clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.05, n_estimators=250,\
    # subsample=1.0, min_samples_split=10, min_samples_leaf=5,\
    # max_depth=4)
    clf.fit(data, target)
    y_pred = clf.predict(data)

    print accuracy_score(y_pred, target)
    joblib.dump(clf, 'model/model.pkl') 
    return 0


def cv_model(data, target):
    #clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.05, n_estimators=250,\
    #  min_samples_split=10, min_samples_leaf=5,\
    # max_depth=4)
    #clf = LogisticRegression(penalty='l1', tol=0.0001, C=1.0, fit_intercept=True, intercept_scaling=1, 
    #    class_weight=None, random_state=None, solver='liblinear', max_iter=100, multi_class='ovr', verbose=0, warm_start=False)
    clf = xgb.XGBClassifier(max_depth = 3, n_estimators = 150, learning_rate = 0.07, reg_alpha = 0.7, reg_lambda = 0.7)
    s = 0
    kf = KFold(len(data), n_folds=3, shuffle = True)
    for train_index, test_index in kf:
        X_train, X_test = data[train_index], data[test_index]
        y_train, y_test = target[train_index], target[test_index]
        clf.fit(X_train, y_train)
        y_pred = clf.predict(X_test)
        s+= accuracy_score(y_pred, y_test)
    print s / 3.0


def splitParagraph(para):
    para = para.strip()
    clf = joblib.load('model/model.pkl')
    res = []
    st_col = extract_features()
    st_col.collect_features(para)
    v = np.asarray(st_col.get_features())

    pred = clf.predict(v)
    prev = 0
    pos = -1
    for num, sym in enumerate(para):
        if sym == '.' or sym == '!' or sym == '?':
            pos += 1
            if (pred[pos] == 1):
                s = para[prev:num + 1]
                res.append(remove_spaces(s))
                prev = num + 1
    if not res or para[-1] != '.' and para[-1] != '!' and para[-1] != '?':
        res.append(remove_spaces(para[prev:]))

    return {'Paragraph': para, 'Sentences': res}


def dump_to_file(file_name, gs):
    with open(file_name, 'w') as f_out:
        for i in range(len(gs)):
            d = splitParagraph(gs[i]['Paragraph'])
            s = json.dumps(d, ensure_ascii=False).encode('utf8')
            f_out.write(s + "\n")
    return 0

def dump_out(arr_lines):
    for i in range(len(arr_lines)):
        d = splitParagraph(arr_lines[i].decode('utf8'))
        s = json.dumps(d, ensure_ascii = False).encode('utf8')
        sys.stdout.write(s + '\n')
    return 0

def get_data(gs):
    data = []
    target = []
    for i in range(len(gs)):
        st_col = extract_features()
        st_col.collect_features(gs[i]['Paragraph'], gs[i]['Sentences'])
        target = target + st_col.get_target()

        if i == 0:
            data = st_col.get_features()
        else:
            data = np.concatenate((data, st_col.get_features()), axis = 0)
    return np.asarray(data, dtype = np.float), np.asarray(target, dtype = np.float)

def read_lines():
    arr_lines = []
    for line in sys.stdin:
        arr_lines.append(line)
    dump_out(arr_lines)
    return 0 

def main():
    #f = open(sys.argv[1])
    #gs = [json.loads(s) for s in f]
    read_lines()
    #data, target = get_data(gs)
    #print data.shape, target.shape
    #print data[10:50, 0:3]
    #fit_model(data, target)
    #cv_model(data, target)
    return 0 


if __name__ == '__main__':
    main()



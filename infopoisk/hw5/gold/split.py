import re
import sys, json
from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.externals import joblib
import numpy as np
from sklearn.metrics import accuracy_score
from sklearn.cross_validation import KFold

def findOccurences(s, ch):
    return [i for i, letter in enumerate(s) if letter == ch]

class extract_features:
    def __init__(self):
        self.is_next_letter_up = []
        self.is_next_space = []
        self.len_prev_word = []
        self.count_words_to_prev_point = []
        self.is_prev_word_start_up_letter = []
        self.is_prev_letter_point = []
        self.is_last_symbol = []
        self.mark = []

    def collect_features(self, par, sen):
        par = par.strip()
        ind_par = findOccurences(par, '.')
        ind_true = []
        pos = 0
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
            if ind + 1 < len(par) and par[ind + 1].isupper():
                self.is_next_letter_up.append(1)
            else:
                self.is_next_letter_up.append(0)

            if ind + 1 < len(par) and par[ind + 1] == ' ':
                self.is_next_space.append(1)
            else:
                self.is_next_space.append(0)

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

            if ind - 1 >= 0 and par[ind - 1] == '.':
                self.is_prev_letter_point.append(1)
            else:
                self.is_prev_letter_point.append(0)

            prev_ind = ind

    def pick_features(self):
        return [self.is_next_letter_up, self.is_next_space, self.len_prev_word, \
        self.count_words_to_prev_point, self.is_prev_word_start_up_letter, \
        self.is_prev_letter_point, self.is_last_symbol]

    def get_target(self):
        return self.mark

    def get_features(self):
        return np.asarray(self.pick_features(), dtype = np.float).T
            
        


def fit_model(data, target):
    clf.fit(data, target)
    y_pred = clf.predict(data)
    print accuracy_score(y_pred, target)
    joblib.dump(clf, 'model/model.pkl') 
    return 0

def cv_model(data, target):
    clf = GradientBoostingClassifier(loss='deviance', learning_rate=0.1, n_estimators=200,\
     subsample=1.0, min_samples_split=2, min_samples_leaf=3,\
     max_depth=3)
    kf = KFold(len(data), n_folds=3, shuffle = True)
    for train_index, test_index in kf:
        X_train, X_test = data[train_index], data[test_index]
        y_train, y_test = target[train_index], target[test_index]
        clf.fit(X_train, y_train)
        y_pred = clf.predict(X_test)
        print accuracy_score(y_pred, y_test)


def splitParagraph(para):
    clf = joblib.load('model/model.pkl')

    res = []

    cands = para.split('.')
    r = cands[0]
    for c in range(1, len(cands)):
        if cands[c].startswith(' '):
            res.append(r + '.')
            r = cands[c]
        else:
            r += '.' + cands[c]
    res.append(r)

    return {'Paragraph': para, 'Sentences': res}


def main():
    fg = open(sys.argv[1])
    gs = [json.loads(s) for s in fg.readlines()]
    f_out = open('broken_gold.json', 'w')

    data = None
    target = []

    for i in range(len(gs)):
        st_col = extract_features()
        st_col.collect_features(gs[i]['Paragraph'], gs[i]['Sentences'])
        #data = data + st_col.get_features()
        target = target + st_col.get_target()
        if i == 0:
            data = st_col.get_features()
        else:
            data = np.concatenate((data, st_col.get_features()), axis = 0)
        #print data
        #par1 = gs[i]['Paragraph']
        #par2 = gs[i]['Sentences']
        #print par2
        #if par1 == ' '.join(par2):
        #   print True
        #else: print False
        #par_list.append(gs[i]['Paragraph'])
        #d = splitParagraph(gs[i]['Paragraph'])
        #s = json.dumps(d, ensure_ascii = False)
        #f_out.write(s.encode('utf8') + "\n")
    #print data.shape
    #print len(target)
    data = np.asarray(data, dtype = np.float)
    target = np.asarray(target, dtype = np.float)
    pos_in_target = 0
    for i in range(len(gs)):
        ind = 0
        for j in range(len(gs[i]['Paragraph']))
            if gs[i]['Paragraph'][j] == '.':
                if target[pos_in_target] == 1:
                    
    #print data.shape
    print target
    cv_model(data, target)
    f_out.close()  
    return 0 


if __name__ == '__main__':
    main()



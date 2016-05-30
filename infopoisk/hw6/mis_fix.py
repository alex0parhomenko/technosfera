# -*- coding: utf-8 -*- 
import numpy as np
import random as rm
import time
import pickle
import string
import random
import sys
sys.setrecursionlimit(10000)
table = string.maketrans("","")
all_requests = {}
unique_right_words = set()
valid_requests = []
# Модель языка - статистика изменения/удаления/добавления буквы, полученная по запросам
#Модель ошибок - вероятность того что пользователь напишет запрос orig когда хотел написать запрос fix 
#Statistic blog

class Node:    
    def __init__(self, string = None, arr = [], is_leaf = False):
        self.string = string
        self.arr = arr
        self.is_leaf = is_leaf

root = Node()
#

class load_statistic:
    def __init__(self, replace_file, add_file, del_file, w_freq_file, w1_w2_freq_file, all_symbols_file, unique_words_file, layout_change_file):
        self.replace = None
        self.delete = None
        self.add = None
        self.w_freq = None
        self.w1_w2_freq = None
        self.layout_change = None
        self.all_symbols = None
        self.unique_words = None
        self.replace_file = replace_file
        self.add_file = add_file
        self.del_file = del_file
        self.w_freq_file = w_freq_file
        self.w1_w2_freq_file = w1_w2_freq_file
        self.layout_change_file = layout_change_file
        self.all_symbols_file = all_symbols_file
        self.unique_words_file = unique_words_file
        self.dict = {}

    def load(self): 
        #ss = time.clock()
        with open(self.w_freq_file, 'rb') as f:
            self.w_freq = pickle.load(f)
        with open(self.w1_w2_freq_file, 'rb') as f:
            self.w1_w2_freq = pickle.load(f)
        with open(self.unique_words_file, 'rb') as f:
            self.unique_words = pickle.load(f)

        #print type(self.unique_words)
        self.replace = np.load(self.replace_file)
        self.add = np.load(self.add_file)
        self.delete = np.load(self.del_file)
        self.layout_change = np.load(self.layout_change_file)
        self.all_symbols = np.load(self.all_symbols_file)
        for pos, s in enumerate(self.all_symbols):
            self.dict[s] = pos
        #print time.clock() - ss

    def normalize_data(self):
        self.replace = self.replace / np.sum(self.replace)
        self.delete = self.delete / np.sum(self.delete)
        self.add = self.add / np.sum(self.add)


def test_trans(s):
    return s.translate(table, string.punctuation)

def lev(w1, w2):
    start_pos = [len(w1), len(w2)]
    n = len(w1) + 1
    m = len(w2) + 1
    a = np.zeros(shape = (len(w1) + 1, len(w2) + 1), dtype = np.int)
    prev_point = np.zeros(shape = (len(w1) + 1, len(w2) + 1, 2), dtype = np.int)

    for i in range(1, n):
        a[i][0] = i
        prev_point[i][0][0] = i - 1
        prev_point[i][0][1] = 0

    for i in range(1, m):
        a[0][i] = i
        prev_point[0][i][0] = 0
        prev_point[0][i][1] = i - 1

    for i in range(1, n):
        for j in range(1, m):
            if (w1[i - 1] == w2[j - 1]):
                a[i][j] = a[i - 1][j - 1]
                prev_point[i][j][0] = i - 1
                prev_point[i][j][1] = j - 1
            else:
                mini = min(a[i- 1][j], a[i][j - 1], a[i - 1][j - 1])
                a[i][j] = mini + 1
                if mini == a[i - 1][j]:
                    prev_point[i][j][0] = i - 1
                    prev_point[i][j][1] = j
                elif mini == a[i][j - 1]:
                    prev_point[i][j][0] = i
                    prev_point[i][j][1] = j - 1
                elif mini == a[i - 1][j - 1]:
                    prev_point[i][j][0] = i - 1
                    prev_point[i][j][1] = j - 1
    query = []

    while start_pos != [0, 0]:
        x, y = start_pos[0], start_pos[1]
        prev_x, prev_y = prev_point[x][y][0], prev_point[x][y][1]
        if a[x][y] != a[prev_x][prev_y]:
            if prev_x + 1 == x and prev_y == y:
                query.append(['del', w1[prev_x]])
            elif prev_x == x and prev_y + 1== y:
                query.append(['add', w2[prev_y]])
            elif prev_x + 1 == x and prev_y + 1 == y:
                query.append(['replace', w1[prev_x], w2[prev_y]])

        start_pos = [prev_x, prev_y]

    query = list(reversed(query))
    return query



def find_max_prefix(str1, str2):
    pos = 0
    while (pos < min(len(str1), len(str2)) and str1[pos] == str2[pos]):
        pos += 1
    return pos


def add_to_bor(root, string):
    if not root.arr:
        n = Node(string = string, arr = [], is_leaf = True)
        root.arr.append(n)
    else:
        flag = True
        for i in range(len(root.arr)):
            max_pref = find_max_prefix(root.arr[i].string, string)
            if max_pref:
                flag = False
                if max_pref == len(string) and max_pref == len(root.arr[i].string):
                    root.arr[i].is_leaf = True
                elif max_pref < len(string) and max_pref == len(root.arr[i].string):
                    add_to_bor(root.arr[i], string[max_pref:])
                elif max_pref < len(root.arr[i].string) and max_pref == len(string):
                    n = Node(string, [], True)
                    n.arr.append(root.arr[i])
                    n.arr[0].string = n.arr[0].string[max_pref:]
                    root.arr[i] = n
                elif max_pref < len(root.arr[i].string) and max_pref < len(string):
                    n1 = Node(string[max_pref:], [], True)
                    n2 = Node(root.arr[i].string[max_pref:], root.arr[i].arr, root.arr[i].is_leaf)
                    root.arr[i].arr = []
                    root.arr[i].string = root.arr[i].string[:max_pref]
                    root.arr[i].is_leaf = False
                    root.arr[i].arr.append(n1)
                    root.arr[i].arr.append(n2)
                return 0
        if flag:
            n = Node(string, [], is_leaf = True)
            root.arr.append(n)
    return 0


def find_in_bor(root, string):
    #print string
    if string == 'le':
        for i in range(len(root.arr)):
            print root.arr[i].string

    for i in range(len(root.arr)):
        if root.arr[i].string == string:
            return True if root.arr[i].is_leaf else False;
        elif string.startswith(root.arr[i].string):
            return find_in_bor(root.arr[i], string[len(root.arr[i].string):])
    return False

def fuzzy_search(root, string, res_string, now_p, min_p, st):
    global valid_requests

    for i in range(len(root.arr)):
        if root.arr[i].string == string:
            if root.arr[i].is_leaf:
                valid_requests.append([res_string + string, now_p])
            fuzzy_search(root.arr[i], u'', res_string + string, now_p, min_p, st)
        else:
            for j in range(len(root.arr[i].string) - 1, len(root.arr[i].string) + 2):
                new_p = now_p
                q = lev(string[:j], root.arr[i].string)
                for action in q:
                    if action[0] == 'replace':
                        pos1, pos2 = st.dict[action[1]], st.dict[action[2]]
                        new_p *= st.replace[pos1][pos2]
                    elif action[0] == 'del':
                        pos = st.dict[action[1]]
                        new_p *= st.delete[pos]
                    elif action[0] == 'add':
                        pos = st.dict[action[1]]
                        new_p *= st.add[pos]

                if new_p >= min_p:
                    if j >= len(string) and root.arr[i].is_leaf:
                        valid_requests.append([res_string + root.arr[i].string, new_p])
                    fuzzy_search(root.arr[i], string[j:], res_string + root.arr[i].string, new_p, min_p, st)
    return



def fuzzy_search_request(request, st):
    global valid_requests
    words = request.split(' ')
    for w in words:
        fuzzy_search(root, w, '', 1, 0.001, st)
        for w1 in valid_requests:
            print w1[0].encode('utf8'), w1[1]
        #print valid_requests
        valid_requests = []
    return

def clear_word_filter(word, st):
    word = word.replace(' ', '')
    if len(word) == 0:
        return None
    word = test_trans(word.encode('utf8')).decode('utf8')
    word = word.lower()
    for s in word:
        if s not in st.all_symbols or not s.isalpha():
            return None
    return word


def add_words_to_bor(words, st):
    global root
    for w in words:
        w = clear_word_filter(w, st)
        if w:
            #print w
            add_to_bor(root, w)
    return

def dfs(root, depth):
    print root.string, depth
    for i in range(len(root.arr)):
        dfs(root.arr[i], depth + 1)
    return 0


def get_unique_words(filename):
    global unique_right_words

    with open(filename, 'r') as f:
        words = []
        for num, line in enumerate(f):
            print num
            line = line.strip()      
            line = test_trans(line)   
            if line.find('\t') != -1:
                f, s = line.split('\t')
                all_requests[f] = s
                words = words + s.split(' ')
            else:
                words = words + line.split(' ')
            if len(words) > 10000:
                unique_right_words = unique_right_words.union(set(words))
                words = []
        unique_right_words = unique_right_words.union(set(words))

    unique_right_words = list(unique_right_words)
    return unique_right_words    

def gen_freq_symbols(all_symbols, unique_words):
    print all_symbols
    if random.randint(0, 100) > 60:
        l = random.randint(1, 15)
        s = ""
        for i in range(l):
            now_symbol = all_symbols[random.randint(0, len(all_symbols) - 1)]
            s = s + now_symbol
        if s not in unique_words:
            return 0, s
        else:
            return 1, s
    else:
        num = random.randint(0, len(unique_words) - 1)
        s = unique_words[num]
        return 1, s


def test_bor():
    global root


    st = load_statistic(replace_file = 'replace.npy', add_file = 'add.npy', del_file = 'del.npy', w_freq_file = 'word_frequency',\
         w1_w2_freq_file = 'w1_w2_frequency', all_symbols_file = 'symbols.npy', unique_words_file = 'unique_words', \
         layout_change_file = 'layout_change_file.npy')
    st.load()
    uniq_words = st.unique_words[1000:2000]
    all_symbols = st.all_symbols
    #print uniq_words[:40]
    for w in uniq_words:
        add_to_bor(root, w)

    for i in range(40):
        s, w = gen_freq_symbols(all_symbols[33:55], uniq_words)
        print w, s
        if int(find_in_bor(root, w)) == s:
            print 'TRUE'
        else:
            print 'FALSE' 



def main():

    rm.seed(time.clock())
    st = load_statistic(replace_file = 'replace.npy', add_file = 'add.npy', del_file = 'del.npy', w_freq_file = 'word_frequency',\
         w1_w2_freq_file = 'w1_w2_frequency', all_symbols_file = 'symbols.npy', unique_words_file = 'unique_words', \
         layout_change_file = 'layout_change_file.npy')
    st.load()
    st.normalize_data()
    #for i in range(100):
    #    print st.unique_words[i]
    #print st.unique_words[:50]
    sss = time.clock()
    add_words_to_bor(st.unique_words, st)
    print time.clock() - sss
    sss = time.clock()

    if u'googll' in st.unique_words:
        print 'TRUEEE'
    print find_in_bor(root, u'unique_words')
    #if u'googl' in st.unique_words:
    #    print 'TRUE IN'
    #if u'coogle' in st.unique_words:
    #    print 'True in'
    #print st.add[st.dict[u'e']]
    #print st.replace[st.dict[u'l']][st.dict[u'e']]
    #test_bor()
    #print find_in_bor(root, u'google')
    #print find_in_bor(root, u'googl')
    print st.add[st.dict[u'l']]
    #print find_in_bor(u'')
    #fuzzy_search_request(u'игля', st)
    #find_in_bor(root, u'google')
    fuzzy_search_request(u'googl', st)
    #fuzzy_search('',)
    #fuzzy_search_request(u'google', st)
    #fuzzy_search_request(u'викепедея', st)
    #fuzzy_search_request(u'лечешая', st)
    #fuzzy_search_request(u'beatiful', st)
    #fuzzy_search_request(u'исполнителейе', st)
    #fuzzy_search_request(u'клодбеще', st)
    #fuzzy_search_request(u'мамы мила ряма', st)
    #fuzzy_search_request(u'папо прешел дамой', st)
    print time.clock() - sss
    return 0

if __name__ == '__main__':
    main()



# -*- coding: utf-8 -*- 
import sklearn as sk
import numpy as np
from tempfile import TemporaryFile
import pickle
from collections import Counter


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


class statistics_create:
    def __init__(self, replace_filename, del_filename, add_filename, symbols_filename, layout_change_file, \
        word_frequency_filename, w1_w2_frequency_filename, unique_w_filename, input_filename):
        self.all_symbols = np.array([u'а', u'б', u'в', u'г', u'д', u'е', u'ж', u'з', u'и', u'й', u'к',\
               u'л', u'м', u'н', u'о', u'п', u'р', u'с', u'т', u'у', u'ф',\
              u'х', u'ц', u'ч', u'ш', u'щ', u'ы', u'ъ', u'ь', u'э', u'ю', u'я',\
              u'a', u'b', u'c', u'd', u'e', u'f', u'g', u'h', u'j', u'k', u'l', u'm', \
              u'n', u'o', u'p', u'q', u'r', u's', u't', u'u', u'v', u'w', u'x', u'y', u'z', u'1', \
              u'2', u'3', u'4', u'5', u'6', u'7', u'8', u'9', u'.', u',', u'+', u'?', u'!', u'-', u'і', u'i', u'ё', u' ', u'ї', u'ғ',\
              u'қ', u'є', u'ә', u'ң', u'0', u'ä', u'ү', u'ұ', u'š', u'ö', u'ü', u';', u'č', u'ө', u'ç', u'ş',\
              u'_', u'\'', u'ż', u'ğ', u'ı', u'á', u'€', u']', u'*', u'ó', u'ђ', u'[', u'ß', u'ў', u'\\'])
        self.layout_change = np.array([[u'q', u'й'], [u'w', u'ц'], [u'e', u'у'], [u'r', u'к'], [u't', u'е'],\
         [u'y', u'н'], [u'u', u'г'], [u'i', u'ш'], [u'o', u'щ'], [u'p', u'з'], [u']', u'х'], [u']', u'ъ'],\
         [u'a', u'ф'], [u's', u'ы'], [u'd', u'в'], [u'f', u'а'], [u'g', u'п'], [u'h', u'р'], [u'j', u'о'], [u'k', u'л'], [u'l', u'д'], [u';', u'ж'],\
          [u'\'', u'э'], [u'z', u'я'], [u'x', u'ч'], [u'c', u'с'], [u'v', u'м'], [u'b', u'и'], [u'n', u'т'],\
           [u'm', u'ь'], [u',', u'б'], [u'.', u'ю'], [u'/', u'.']])
        self.d = {}
        self.replace_symbol = np.zeros((len(self.all_symbols), len(self.all_symbols)))
        self.del_symbol = np.zeros(len(self.all_symbols))
        self.add_symbol = np.zeros(len(self.all_symbols))
        self.w_frequency = Counter()
        self.w1_w2_frequency = Counter()
        self.replace_filename = replace_filename
        self.del_filename = del_filename
        self.add_filename = add_filename
        self.input_filename = input_filename
        self.all_symbols_file = symbols_filename
        self.layout_change_file = layout_change_file
        self.word_frequency_filename = word_frequency_filename
        self.w1_w2_frequency_filename = w1_w2_frequency_filename
        self.unique_w_filename = unique_w_filename

    def filling_dict(self):
        for num, c in enumerate(self.all_symbols):
            self.d[c] = num
        return 0

    def count_replacement(self):
        self.filling_dict()
        with open(self.input_filename, 'r') as f:
            for num, line in enumerate(f):
                line = line.strip().decode('utf8')
                if num % 10000 == 0 and num > 0:
                    print num
                    
                if line.find('\t') != -1:
                    f, s = line.split('\t')
                    f, s = f.strip(), s.strip()

                    words = s.split(u' ')
                    prev_w = ''
                    for num, w in enumerate(words):
                        self.w_frequency[w] += 1
                        if num > 0:
                            self.w1_w2_frequency[prev_w + ' ' + w] += 1
                        prev_w = w

                    query = lev(f, s)

                    for obj in query:
                        if obj[0] == 'replace':
                            #print obj[1].lower(), obj[2].lower()
                            p1, p2 = self.d[obj[1].lower()], self.d[obj[2].lower()]
                            self.replace_symbol[p1][p2] += 1
                        elif obj[0] == 'add':
                            #print obj[1].lower()
                            p1 = self.d[obj[1].lower()]
                            self.del_symbol[p1] += 1
                        elif obj[0] == 'del':
                            #print obj[1].lower()
                            p1 = self.d[obj[1].lower()]
                            self.add_symbol[p1] += 1
                else:
                    words = line.split(u' ')
                    prev_w = ''
                    for num, w in enumerate(words):
                        self.w_frequency[w] += 1
                        if num > 0:
                            self.w1_w2_frequency[prev_w + ' ' + w] += 1
                        prev_w = w
        unique_words = list(self.w_frequency.keys())
        #print len(unique_words)
        np.save(self.replace_filename, self.replace_symbol)
        np.save(self.add_filename, self.add_symbol)
        np.save(self.del_filename, self.del_symbol)
        np.save(self.all_symbols_file, self.all_symbols)
        np.save(self.layout_change_file, self.layout_change)
        with open(self.word_frequency_filename, 'wb') as f:
            pickle.dump(self.w_frequency, f)
        with open(self.w1_w2_frequency_filename, 'wb') as f:
            pickle.dump(self.w1_w2_frequency, f)
        with open(self.unique_w_filename, 'wb') as f:
            pickle.dump(unique_words, f)

        #print self.replace_symbol
        return 0





def main():
    st = statistics_create(replace_filename = 'replace', del_filename = 'del', add_filename = 'add', symbols_filename = 'symbols',\
         layout_change_file = 'layout_change_file', word_frequency_filename = 'word_frequency',\
          w1_w2_frequency_filename = 'w1_w2_frequency', unique_w_filename = 'unique_words', input_filename = 'queries_all.txt')
    st.count_replacement()

    return 0


if __name__ == '__main__':
    main()
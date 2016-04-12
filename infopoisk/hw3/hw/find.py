# -*- coding: utf-8 -*-
import pickle
import docreader
import numpy
import mmh3
import math
import sys
from collections import Counter
from struct import *
import os
from os import lseek, read, write
import re, string, timeit
import time
from string import lower
import solution
import expr

all_words = []

class Node:
    def __init__(self, val = None, r = None, l = None, isleaf = False, neg = False):
        self.r = r
        self.l = l
        self.val = val
        self.isleaf = isleaf
        self.neg = neg


def decode_one_byte(s):
    a = unpack(str(len(s)) + 'B', s)
    return a[0]

def find_word_varbyte(w, d, reverse_ind):
    s = []
    p1 = d[w]
    reverse_ind.seek(int(p1), os.SEEK_SET)
    cou = 0
    res_str = ""
    prev_num = 0
    while (1):
        ss = reverse_ind.read(1)
        num = decode_one_byte(ss)
        if (num == 128 and prev_num > 128):
            break
        elif (num >= 128):
            res_str += ss
            cou += solution.from_varbyte(res_str)
            s.append(cou)
            res_str = ""
        else:
            res_str = res_str + ss
        prev_num = num
    return set(s)


def find_word_simple9(w, d, reverse_ind):
    s = []
    if w not in d:
        t = set()
        return t
    p1 = d[w]
    reverse_ind.seek(int(p1), os.SEEK_SET)
    cou = 0
    res_str = ""
    prev_num = 0
    flag = True
    while (flag):
        ss = reverse_ind.read(4)
        arr = solution.from_simple9(ss)
        for i in range(len(arr)):
            if (arr[i] == 250000000):
                flag = False
                break
            else:
                cou += arr[i]
                s.append(cou)
    return set(s)

def dfs(root, d, reverse_ind, mode):
    if (root.isleaf == True):
        if root.neg == True and mode == 'simple9':
            return all_words.difference(find_word_simple9(root.val, d, reverse_ind))
        elif root.neg == True and mode == 'varbyte':
            return all_words.difference(find_word_varbyte(root.val, d, reverse_ind))
        if root.neg == False and mode == 'simple9':
            return find_word_simple9(root.val, d, reverse_ind)
        elif root.neg == False and mode == 'varbyte':
            return find_word_varbyte(root.val, d, reverse_ind)
    l_set = None
    r_set = None
    if (root == None):
        return None
    else:
        if (root.l != None):
            l_set = dfs(root.l, d, reverse_ind, mode)
        if (root.r != None):
            r_set = dfs(root.r, d, reverse_ind, mode)
    ans = None
    if root.val == '|':
        ans = l_set.union(r_set)
    elif root.val == '&':
        ans = l_set.intersection(r_set)
    if (root.neg == True):
        ans = all_words.difference(ans)
    return ans

def find_simple9(s, urls, d, reverse_ind):
    root = expr.get_tree(s)
    s = dfs(root, d, reverse_ind, mode = 'simple9')
    s = list(s)
    s = sorted(s)
    print len(s)
    for doc_id in s:
        print urls[doc_id - 1]
    return 0

def find_varbyte(s, urls, d, reverse_ind):
    root = expr.get_tree(s)
    s = dfs(root, d, reverse_ind, mode = 'varbyte')
    s = list(s)
    s = sorted(s)
    print len(s)
    for doc_id in s:
        print urls[doc_id - 1]
    return 0

def main():
    global all_words
    urls = []
    f = open('codingtype', 'r')
    codingtype = f.readline()
    f.close()
    fd = open('all_urls', 'r')
    with open('all_urls', 'r') as f:
        for line in f:
            urls.append(line.strip())
    fd.close()
    all_words = set(range(1, len(urls)))

    reverse_ind = open('reverse_index_' + codingtype, "r+")
    d = {}

    fd = os.open('dict', os.O_RDWR)
    s = ''
    while (True):
        s = os.read(fd, 8)
        if (len(s) < 8):
            break
        w, offset = unpack('ii', s)
        d[w] = offset

    os.close(fd)

    while True:
        line = sys.stdin.readline()
        if len(line) ==0:
            break
        print line.strip()
        line = line.decode('utf-8').lower().encode('utf8')
        if codingtype == 'varbyte':
            find_varbyte(line, urls, d, reverse_ind)
        elif codingtype == 'simple9':
            find_simple9(line, urls, d, reverse_ind)

    reverse_ind.close()

if __name__ == '__main__':
    main()




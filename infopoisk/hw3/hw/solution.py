# -*- coding: utf-8 -*-
import docreader
import numpy
import math
import sys
import mmh3
from collections import Counter
from struct import *
import os
import pickle
import re, string, timeit
import time
import numpy as np
SPLIT_RGX = re.compile(r'\w+', re.U)


def extract_words(text):
    words = re.findall(SPLIT_RGX, text)
    return map(lambda s: s.lower(), words)

def to_varbyte(x):
    result_string = ''
    num_iter = 0
    if (x == 0):
        return pack('B', 128)
    while (x > 0):
        balance = x % 128
        if (num_iter == 0):
            balance += 128
        # Use b to get signed char
        result_string = pack('B', balance) + result_string
        x /= 128
        num_iter += 1
    return result_string

def from_varbyte(s):
    arr = unpack(str(len(s)) + 'B', s)
    res = 0
    pos = 1
    for val in reversed(arr):
        if (pos == 1):
            val -= 128
        res += val * pos
        pos *= 128
    return res


def code_arr(arr):
    switcher={
        1 : 28,
        2 : 14,
        3 : 9,
        4 : 7,
        5 : 5,
        7 : 4,
        9 : 3,
        14 : 2,
        28 : 1,
    }
    reverse_switcher={
        28: 0,
        14: 1,
        9: 2,
        7: 3,
        5: 4,
        4: 5,
        3: 6,
        2: 7,
        1: 8,
    }
    cou_bits = switcher.get(len(arr), 'nothing')
    res_num = 0
    for i, num in enumerate(arr):
        res_num = res_num * (1 << cou_bits) + num
    n = reverse_switcher.get(len(arr), 'nothing')
    res_num += (1 << 28) * n

    return pack('I', res_num)



def to_simple9(arr):
    switcher = {
        1 : 28,
        2 : 14,
        3 : 9,
        4 : 7,
        5 : 5,
        7 : 4,
        9 : 3,
        14 : 2,
        28 : 1,
    }
    prev_coding = {
        1 : -1,
        2 : 1,
        3 : 2,
        4 : 3,
        5 : 4,
        7 : 5,
        9 : 7,
        14 : 9,
        28 : 14,
    }
    new_arr = []
    arr_powers = [0, 2, 4, 8, 16, 32, 128, 512, 16384, 268435456]
    arr_bits = [1, 2, 3, 4, 5, 7, 9, 14, 28]
    bits_for_coding = []
    maxi = -np.inf
    if (len(arr) == 1 and arr[0] >= 16384):
        s = code_arr(arr)
        return [], s

    for i in range(len(arr)):
        l = 0
        r = len(arr_powers)
        num = arr[i]
        while (r - l > 1):
            mid = (l + r) / 2
            if (arr_powers[mid] > num):
                r = mid
            else:
                l = mid
        bits_for_coding.append(arr_bits[l])
        maxi = max(maxi, arr_bits[l])
        #print maxi
        p = switcher.get(len(bits_for_coding), 'nothing')
        if (p == 'nothing'):
            continue
        if (maxi <= p and len(bits_for_coding) != 28):
            continue
        else:
            #print 'i here', i
            cou = 0
            if (len(bits_for_coding) == 28):
                if (maxi <= 1):
                    cou = 28
                else:
                    cou = 14
            else:
                cou = prev_coding.get(len(bits_for_coding), 'nothing')
            s = code_arr(arr[:cou])
            return arr[cou:], s
    return None

def from_simple9(s):
    switcher={
        0:28,
        1:14,
        2:9,
        3:7,
        4:5,
        5:4,
        6:3,
        7:2,
        8:1,
    }
    switcher_bits={
        1 : 28,
        2 : 14,
        3 : 9,
        4 : 7,
        5 : 5,
        7 : 4,
        9 : 3,
        14 : 2,
        28 : 1,
    }
    num = unpack('I', s)[0]
    cou = num >> 28
    #print cou
    num -= cou * (1 << 28)
    cou = switcher.get(cou, 'nothing')
    cou_bits = switcher_bits.get(cou, 'nothing')
    #print cou
    answer = []
    for i in range(cou):
        balance = num % (1 << cou_bits)
        answer.append(balance)
        num /= (1 << cou_bits)
    answer = list(reversed(answer))
    return answer

def index_varbyte():
    all_offsets = []
    all_offsets_jump = []
    now_offset = 0
    offset_jump = 0
    repeat = 0
    prev_w = -1
    prev_doc = 0
    prev_doc_jump = 0

    reverse_index = os.open("reverse_index_varbyte", os.O_RDWR | os.O_CREAT)
    jumps = os.open("jumps", os.O_RDWR | os.O_CREAT)

    with open("sort_pairs", "r") as f:
        for line in f:
            now_w, doc_n = line.split(' ')
            now_w = int(now_w)
            doc_n = int(doc_n)
            if (prev_w != now_w):
                if (now_offset != 0):
                    os.write(reverse_index, to_varbyte(0))
                    os.write(jumps, to_varbyte(0))
                    now_offset += 1
                    offset_jump += 1
                all_offsets.append(now_offset)
                all_offsets_jump.append(offset_jump)
                repeat = 1
                prev_doc = 0
                prev_doc_jump = 0

            if (repeat % 50 == 0):
                s = to_varbyte(int(doc_n - prev_doc_jump))
                os.write(jumps, s)
                s = to_varbyte(int(now_offset))
                os.write(jumps, s)
                offset_jump += len(s)
                prev_doc_jump = doc_n

            s = to_varbyte(int(doc_n - prev_doc))
            now_offset += len(s)
            os.write(reverse_index, s)

            prev_w = now_w
            prev_doc = doc_n
            repeat += 1

    f.close()
    os.close(reverse_index)
    os.close(jumps)
    return all_offsets, all_offsets_jump

def index_simple9():
    all_offsets = []
    all_offsets_jump = []
    now_dists = []
    now_jump_dists = []
    now_offset = 0
    offset_jump = 0
    repeat = 0
    prev_w = -10
    prev_doc = 0
    prev_doc_jump = 0

    reverse_index = os.open("reverse_index_simple9", os.O_RDWR | os.O_CREAT)
    jumps = os.open("jumps", os.O_RDWR | os.O_CREAT)
    k = 0
    with open("sort_pairs", "r") as f:
        for line in f:
            now_w, doc_n = line.split(' ')
            now_w = int(now_w)
            doc_n = int(doc_n)
            if (prev_w != now_w):
                if (len(all_offsets) != 0):
                    #control_arr = [250000000] * 29
                    now_dists = now_dists + [250000000] * 30

                    while (len(now_dists) > 29):
                        now_dists, s = to_simple9(now_dists)

                        now_offset += len(s)
                        os.write(reverse_index, s)
                    now_dists = []

                all_offsets.append(now_offset)
                all_offsets_jump.append(offset_jump)
                repeat = 1
                prev_doc = 0
                prev_doc_jump = 0

            #if (repeat % 50 == 0):
            #    now_jump_dists.append([int(doc_n - prev_doc_jump), int(now_offset)])
            #    prev_doc_jump = doc_n
            now_dists.append(int(doc_n -prev_doc))
            if (len(now_dists) >= 30):
                #prev = now_dists
                now_dists, s = to_simple9(now_dists)
                os.write(reverse_index, s)
                now_offset += len(s)

            prev_w = now_w
            prev_doc = doc_n
            repeat += 1
        #control_arr = [250000000] * 29
        now_dists = now_dists + [250000000] * 30
        while (len(now_dists) != 29):
            #prev = now_dists
            now_dists, s = to_simple9(now_dists)
            now_offset += len(s)
            os.write(reverse_index, s)

    f.close()
    os.close(reverse_index)
    os.close(jumps)
    return all_offsets, all_offsets_jump

def main():
    urls = []
    all_words = []
    docs = []
    all_offsets = []
    all_offsets_jump = []
    ll = 0
    f = open('pairs', 'w')
    word_id = {}
    now_id = 0
    for archive in sys.argv[2:]:
        u, d = docreader.main(archive)
        pos = 0
        for doc_num in d:
            doc_num = extract_words(doc_num)
            doc_num = set(doc_num)
            for w in doc_num:
                if (w not in word_id):
                    all_words.append(w)
                    word_id[w] = str(now_id)
                    now_id += 1
                f.write(word_id[w] + " " + str(len(urls) + pos + 1) + "\n")
            pos +=1
        urls = urls + u
    f.close()
    os.system("sort --parallel=100000 -n -k1,1 -k2,2 -o sort_pairs pairs")

    if (sys.argv[1] == 'varbyte'):
        all_offsets, all_offsets_jump = index_varbyte()
        f = open('codingtype', 'w')
        f.write('varbyte')
        f.close()
    else:
        all_offsets, all_offsets_jump = index_simple9()
        f = open('codingtype', 'w')
        f.write('simple9')
        f.close()

    fd = os.open("dict", os.O_RDWR | os.O_CREAT)
    for w in all_words:
        s1 = pack('i', mmh3.hash(w.encode('utf8')))
        s2 = pack('i', all_offsets[(int)(word_id[w])])
        s = s1 + s2
        os.write(fd, s)
    os.close(fd)
    f = open("all_urls", "w")
    for u in urls:
        f.write(u + "\n")
    f.close()
    return 0

if __name__ == '__main__':
    main()
#!/usr/bin/env python
import sys
import re
import mmh3
import numpy as np
import os
from docreader import DocumentStreamReader
import document_pb2
import struct
import gzip


class MinshinglesCounter:
    SPLIT_RGX = re.compile(r'\w+', re.U)

    def __init__(self, window=5, n=20):
        self.window = window
        self.n = n

    def count(self, text):
        words = MinshinglesCounter._extract_words(text)
        shs = self._count_shingles(words)
        mshs = self._select_minshingles(shs)

        if len(mshs) == self.n:
            return mshs

        if len(shs) >= self.n:
            return sorted(shs)[0:self.n]

        return None

    def _select_minshingles(self, shs):
        buckets = [None]*self.n
        for x in shs:
            bkt = x % self.n
            buckets[bkt] = x if buckets[bkt] is None else min(buckets[bkt], x)

        return filter(lambda a: a is not None, buckets)

    def _count_shingles(self, words):
        shingles = []
        for i in xrange(len(words) - self.window):
            h = mmh3.hash(' '.join(words[i:i+self.window]).encode('utf-8'))
            shingles.append(h)
        return sorted(shingles)

    @staticmethod
    def _extract_words(text):
        words = re.findall(MinshinglesCounter.SPLIT_RGX, text)
        return words


def main():
    mhc = MinshinglesCounter()
    archive_cou = len(sys.argv) - 1

    urls = []
    cou_sh_url = np.zeros(100100)
    hash_arr = []
    doc_id = 0
    for archive_id in range(archive_cou):
        reader = DocumentStreamReader(str(sys.argv[archive_id + 1]))
        for doc in reader:
            now_hash = mhc.count(doc.text)
            urls.append(doc.url)
            if (now_hash is None):
                doc_id += 1
                continue
            now_hash = set(now_hash)
            for hh in now_hash:
                if (hh is not None):
                    hash_arr.append([hh, doc_id])
                    cou_sh_url[doc_id] += 1
            doc_id += 1


    hash_arr = sorted(hash_arr)
    os.system('touch doc')
    with open('doc', 'w') as f:
        now_pos = []
        for i in range(1, len(hash_arr)):
            if (hash_arr[i][0] == hash_arr[i - 1][0]):
                if (not now_pos):
                    now_pos.append(i - 1)
                    f.write(str(hash_arr[i - 1][1]) + ' ' + str(hash_arr[i][1]) + '\n')
                else:
                    for p in now_pos:
                        f.write(str(hash_arr[p][1]) + ' ' + str(hash_arr[i][1]) + '\n')
                now_pos.append(i)
            else:
                now_pos = []

    f.close()

    os.system("sort --parallel=100000 -k 1,1n -k 2,2n -no sort_doc doc")
    del hash_arr[:]

    cnt = 0
    with open('sort_doc') as file:
        prev_f = ""
        prev_s = ""
        prev_line = ""
        cou = 0
        for line in file:
            cnt += 1
            line = line.strip()
            f, s = line.split(' ')
     
            if (f == prev_f and s == prev_s):
                cou += 1
            else:
                if (prev_line.find(' ') != -1):
                    now_f, now_s = prev_line.split(' ')
                    now_f = int(now_f)
                    now_s = int(now_s)
                    

                    if (cou * 1.0 / (cou_sh_url[now_f] + cou_sh_url[now_s] - cou) > 0.75):
                        print urls[now_f] + " " + urls[now_s] + " " + str(int(round(cou * 1.0 / (cou_sh_url[now_f] + cou_sh_url[now_s] - cou)*100.)))
                cou = 1
                prev_line = line
                prev_f = f
                prev_s = s

    file.close()


if __name__ == '__main__':
    main()

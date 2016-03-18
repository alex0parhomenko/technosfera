#!/usr/bin/env python

import document_pb2
import struct
from os import listdir
import subprocess
import string
import sys

table = string.maketrans("","")

class DocumentStreamReader:
    def __init__(self, stream):
        self.stream = stream

    def __iter__(self):
        while True:
            sb = self.stream.read(4)
            if sb == '':
                return

            size = struct.unpack('i', sb)[0]
            msg = self.stream.read(size)
            doc = document_pb2.document()
            doc.ParseFromString(msg)
            yield doc


def main(files):
    zcat = subprocess.Popen(['zcat'] + files, stdout=subprocess.PIPE)

    reader = DocumentStreamReader(zcat.stdout)
    docs = []
    urls = []
    for doc in reader:
        docs.append(((doc.text).encode('utf8')).translate(table, string.punctuation))
        urls.append(doc.url)
    return docs, urls


if __name__ == '__main__':
    main()

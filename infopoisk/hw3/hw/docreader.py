#!/usr/bin/env python

import document_pb2
import struct
import gzip
import sys


class DocumentStreamReader:
    def __init__(self, path):
        if path.endswith('.gz'):
            self.stream = gzip.open(path, 'rb')
        else:
            self.stream = open(path, 'rb')

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


def main(path):
    reader = DocumentStreamReader(path)
    urls = []
    docs = []
    for doc in reader:
        docs.append(unicode(doc.text).lower())
        #print type(doc.text.encode('utf8').lower())
        urls.append(doc.url)
    return urls, docs


if __name__ == '__main__':
    main()

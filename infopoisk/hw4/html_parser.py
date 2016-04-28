import sys
import base64
import random
import chardet
import codecs
import re, string, timeit
from re import sub

from nltk.corpus import stopwords
from nltk.stem import SnowballStemmer
from HTMLParser import HTMLParser

stemmer = SnowballStemmer("russian");
table = string.maketrans("","")


def test_trans(s):
    return s.translate(table)


class SpamHTMLParser(HTMLParser):
    global table
    def __init__(self):
        HTMLParser.__init__(self)
        self.__text = []
        self.__titletext = []
        self.__atext = []
        self.__strong_text = []
        self.__em_text = []
        self.title_tag = 0
        self.strong_tag = 0
        self.em_tag = 0
        self.a = 0

    def handle_data(self, data):
        text = data.strip()

        if len(text) > 0:
            text = sub('[ \t\r\n]+', ' ', text)
            self.__text.append(text + ' ')
            if (self.title_tag == 1):
                self.__titletext.append(text + ' ')
            if (self.a == 1):
                self.__atext.append(text + ' ')
            if self.strong_tag == 1:
                self.__strong_text.append(text + ' ')
            if self.em_tag == 1:
                self.__em_text.append(text + ' ')

    def handle_starttag(self, tag, attrs):
        if tag == 'p':
            self.__text.append('\n\n')
        elif tag == 'br':
            self.__text.append('\n')
        elif tag == 'title':
            self.title_tag = 1
        elif tag == 'a':
            self.a = 1
        elif tag == 'strong':
            self.strong_tag = 1
        elif tag == 'em':
            self.em_tag = 1
        
            
    def handle_endtag(self, tag):
        if tag == 'title':
            self.title_tag = 0
        elif tag == 'a':
            self.a = 0
        elif tag == 'strong':
            self.strong_tag = 0
        elif tag == 'em':
            self.em_tag = 0

    def handle_startendtag(self, tag, attrs):
        if tag == 'br':
            self.__text.append('\n\n')


    def text(self):
        return ''.join(self.__text).strip()
    
    def titletext(self):
        return ''.join(self.__titletext).strip()
    
    def atext(self):
        return ''.join(self.__atext).strip()

    def strongtext(self):
        return ''.join(self.__strong_text).strip()

    def emtext(self):
        return ''.join(self.__em_text).strip()
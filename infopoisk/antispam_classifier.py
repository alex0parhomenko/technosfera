# coding: utf-8

import sys
import random
import base64


def is_spam(pageInb64, url):
    original = base64.b64decode(pageInb64)
    # check url 
    if random.random() > 0.5 : return 0
    return 1
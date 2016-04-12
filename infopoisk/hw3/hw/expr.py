# -*- coding: utf-8 -*-
from copy import copy
import mmh3
ind = 0
def get_lexems(s):
    lexems = []
    s = s.strip()
    s = s.replace(' ', '')
    arr = ['(', '&', '|', '!', ')']
    w = ''
    for i in range(len(s)):
        if (s[i] in arr):
            if (len(w) != 0):
                lexems.append(w)
                w = ''
            lexems.append(s[i])
        else:
            w += s[i]
    if (len(w) != 0):
        lexems.append(w)
    return lexems

class Node:
    def __init__(self, val = None, r = None, l = None, isleaf = False, neg = False):
        self.r = r
        self.l = l
        self.val = val
        self.isleaf = isleaf
        self.neg = neg

def get_tree(s):
    global ind
    ind = 0
    return process_expr(get_lexems(s), '|')

def process_expr(lexems, mode):
    global ind
    if (ind >= len(lexems)):
            return None
    if mode == '|':
        l = process_expr(lexems, '&')
        root = l
        while (ind < len(lexems) and lexems[ind] == '|'):
            ind += 1
            r = process_expr(lexems, '&')
            root = Node(val = '|', l = l, r = r)
            l = root
        return root
    elif mode == '&':
        l = process_expr(lexems, 'w')

        root = l
        while (ind < len(lexems) and lexems[ind] == '&'):
            ind += 1
            r = process_expr(lexems, 'w')
            root = Node(val = '&', l = l, r = r)
            l = root
        return root
    elif mode == 'w':
        neg = False
        if (lexems[ind] == '!'):
            neg = True
            ind += 1
        if (lexems[ind] == '('):
            ind+=1
            root = process_expr(lexems, '|')
            root.neg = neg
            ind +=1
            return root
        else:
            root = Node(val = mmh3.hash(lexems[ind]), r = None, l = None, neg = neg, isleaf = True)
            ind += 1
            return root
        



def main():

    dfs(root)
    return 0
    print root.val
    l = root.l
    r = root.r
    print r.val, l.val
    l = r.l
    r = r.r
    print l.val, r.val
    return 0

if __name__ == '__main__':
    main()
#!/usr/bin/env python
# vim: set fileencoding=UTF-8 :

"""Converts any given Textile input file into HTML.

"""
from __future__ import division
from __future__ import print_function

__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '5 March 2005'
__version__ = '1.0'

import os.path
import sys

dir = '.'
try: raise RuntimeError
except:
    x = sys.exc_info()[2]
    prog = x.tb_frame.f_code.co_filename
    dir = os.path.dirname(prog)

sys.path.append(os.path.join(dir, 'textile-2.1.4'))
import textile

if __name__ == '__main__':
    if len(sys.argv) < 2: raise KeyError('not enough arguments')
    
    file = open(sys.argv[1], 'r')
    if not file: raise Exception('unable to open %s', str(file))
    
    contents = file.read()
    if not contents: raise Exception('unable to read %s', str(file))
    
    html = textile.textile(contents)
    print(html)


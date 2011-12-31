#! /usr/bin/env python

# find nth digit of pi for which n appears there
# Since Dec 7 2011

import os
import sys
infile = sys.argv[1]

N = os.path.getsize(infile);
fd = open(infile,'r')
#fd = open('test.txt','r')
for n in xrange(1,N):
    fd.seek(n + 1)
    str1 = str(n)
    str2 = fd.read(len(str1))
    #print str1, str2 
    if str1 == str2:
        print str1

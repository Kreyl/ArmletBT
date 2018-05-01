#!/usr/bin/python
#
# Basic settings for Music Processor for "Dark Tower: All Hail" LARP.
#
from datetime import datetime
from os.path import dirname, join, realpath
from sys import argv

def currentTime():
    return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def getFileName(name):
    return join(dirname(realpath(argv[0])), name)

REASON_ID_START = 1
REASON_ID_END = 127
REASON_ID_RANGE = xrange(REASON_ID_START, REASON_ID_END + 1)

CHARACTER_ID_START = REASON_ID_END + 1 # 128
CHARACTER_ID_END = 199
CHARACTER_IDS = xrange(CHARACTER_ID_START, CHARACTER_ID_END + 1)

assert 0 < REASON_ID_START < REASON_ID_END < CHARACTER_ID_START < CHARACTER_ID_END

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

CHARACTER_ID_START = 128
CHARACTER_ID_END = 199
CHARACTER_IDS = xrange(CHARACTER_ID_START, CHARACTER_ID_END + 1)

SOURCE_ID_START = 200
SOURCE_ID_END = 499

MAX_ID = SOURCE_ID_END

assert 0 < CHARACTER_ID_START < CHARACTER_ID_END < SOURCE_ID_START < SOURCE_ID_END == MAX_ID

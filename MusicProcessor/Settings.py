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

RESERVED_WEIGHT = 0
LOCATION_WEIGHT = 40
CHARACTER_WEIGHT = 80
MASTERKA_WEIGHT = 240
DEATH_WEIGHT = 280

MASTER_ID_START = 1
MASTER_ID_END = 6
MASTER_IDS = xrange(MASTER_ID_START, MASTER_ID_END + 1)

PLACEHOLDER_ID_START = 11
PLACEHOLDER_ID_END = 40
PLACEHOLDER_IDS = xrange(PLACEHOLDER_ID_START, PLACEHOLDER_ID_END + 1)

LOCATION_ID_START = 41
LOCATION_ID_END = 47
LOCATION_IDS = xrange(LOCATION_ID_START, LOCATION_ID_END + 1)

CHARACTER_ID_START = 201
CHARACTER_ID_END = 279
CHARACTER_IDS = xrange(CHARACTER_ID_START, CHARACTER_ID_END + 1)

MAX_ID = 320

assert 0 < MASTER_ID_START < MASTER_ID_END \
		 < PLACEHOLDER_ID_START < PLACEHOLDER_ID_END < LOCATION_ID_START < LOCATION_ID_END \
		 < CHARACTER_ID_START < CHARACTER_ID_END <= MAX_ID

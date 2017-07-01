#!/usr/bin/python
#
# JoinRPG.ru API
#
from csv import DictReader
from os.path import dirname, join, realpath
from urllib2 import urlopen
from sys import argv

try:
    from CSVable import CSVObjectReader
except ImportError, ex:
    CSVObjectReader = None

ENCODING = 'utf-8'

ALL_ROLES_URL = 'http://joinrpg.ru/%d/characters/activetoken?Token=%s'

TOKEN_FILE_NAME = 'joinrpg.key'
DUMP_FILE_NAME = 'joinrpg-%d.csv'
DUMP_FILE_NAME_1251 = 'joinrpg-%d-1251.csv'

def getFileName(name):
    return join(dirname(realpath(argv[0])), name)

def _getData(gameID, token = None, tokenFileName = None, dumpCSV = False, dumpCSV1251 = False):
    token = (token or open(tokenFileName or getFileName(TOKEN_FILE_NAME)).read()).strip()
    url = ALL_ROLES_URL % (gameID, token)
    data = urlopen(url).read() # open(getFileName(DUMP_FILE_NAME), 'rb').read()
    if dumpCSV:
        with open(dumpCSV if dumpCSV is not True else getFileName(DUMP_FILE_NAME % gameID), 'wb') as f:
            f.write(data)
    if dumpCSV1251:
        with open(dumpCSV1251 if dumpCSV1251 is not True else getFileName(DUMP_FILE_NAME_1251 % gameID), 'wb') as f:
            f.write(data.decode(ENCODING).encode('windows-1251'))
    return data.splitlines()

def getAllCharacters(gameID, token = None, tokenFileName = None, dumpCSV = False, dumpCSV1251 = False):
    """Returns all characters for a game with the specified ID as iterator of dictionaries."""
    reader = DictReader(_getData(gameID, token, tokenFileName, dumpCSV, dumpCSV1251))
    return (dict((field.decode(ENCODING), row[field].decode(ENCODING)) for field in reader.fieldnames) for row in reader)

if CSVObjectReader:
    def getAllCharactersAsObjects(gameID, csvAbleClass, token = None, tokenFileName = None, dumpCSV = False, dumpCSV1251 = False):
        """Returns all characters for a game with the specified ID as iterator of CSVable objects."""
        return CSVObjectReader(_getData(gameID, token, tokenFileName, dumpCSV, dumpCSV1251), csvAbleClass, True, ENCODING)

def main():
    getAllCharacters(int(argv[1]), dumpCSV = True, dumpCSV1251 = True)

if __name__ == '__main__':
    main()

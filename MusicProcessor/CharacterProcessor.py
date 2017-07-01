#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Character Processor for "Dark Tower: All Hail" LARP.
#
# - Downloads CSV with all characters for a game
# - Extracts character names and data from CSV
# - Loads, verifies and updates Characters.csv file
#
# Usage: python CharacterProcessor.py
#
from collections import OrderedDict
from os.path import isfile
from re import compile as reCompile
from traceback import format_exc

from CSVable import CSVable, CSVObjectReader, CSVObjectWriter
from joinrpg import getAllCharactersAsObjects

from Settings import currentTime, getFileName, CHARACTER_ID_START, CHARACTER_ID_END, CHARACTER_IDS

ENCODING = 'windows-1251'

GAME_ID = 110

CHARACTERS_CSV = 'Characters.csv'

CHARACTERS_CSV_HEADER = '''\

Characters.csv for "Dark Tower: All Hail" LARP.

Characters table for ArmLet initialization.

Generated, updated from JoinRPG and used by CharacterProcessor.py
to track persistent unique character Reason IDs.

!!! DO NOT EDIT !!!

Generated at %s

'''

class CharacterError(Exception):
    pass

class CharacterCSVable(CSVable):
    CSV_FIELDS = OrderedDict((
        ( 'rID', 'rID'),
        (u'1344 Имя на браслете', 'shortName'),
        (u'Персонаж', 'longName'),
        (u'Нью-Йорк?', 'isNY'),
        (u'1200 Внутренний Доган', 'dogan'),
        (u'1346 Мэнни', 'isManni'),
        (u'1203 Связи ка-тета', 'kaTet'),
        (u'1399 Стартовое число очков действия', 'nAction'),
        (u'1622 Музыка прислана?', 'hasMusic')
    ))

    DOGAN = OrderedDict(((u'Служба Алому Королю', -2), (u'Алое Колебание', -1), (u'Нейтралитет', 0), (u'Белое Колебание', 1), (u'Путь Белизны', 2)))

    NO = 0
    UNCHECKED = 1
    CHECKED = 2
    HAS_MUSIC = OrderedDict((('', NO), (u'да, не проверена', UNCHECKED), (u'да, проверена', CHECKED)))

    SEPARATORS = reCompile('[,; ]*')

    KA_TET_SEP = ':'

    CHARACTERS = OrderedDict()
    RIDS = set()
    SHORT_NAMES = set()
    LONG_NAMES = set()

    def addSortKey(self):
        """Sort characters by last name in short name, then initial."""
        name = self.shortName.strip()
        return (name[1:], name[0]) if name else ('Z', 'Z')

    def getKaTet(self):
        return tuple(self.kaTet.split(self.KA_TET_SEP)) if self.kaTet else ()

    def setKaTet(self, kaCharacters):
        self.kaTet = self.KA_TET_SEP.join(kaCharacters)

    def processNames(self):
        """Process and validate shortName and longName."""
        self.shortName = self.shortName.strip()
        if not self.shortName:
            raise CharacterError("Character short name is empty")
        try:
            self.shortName = self.shortName.encode('ascii')
        except UnicodeError:
            assert False, "Character short name is not ASCII: %r" % self.shortName
        assert self.shortName.isalpha(), "Character short name is not alphabetic: %s" % self.shortName
        assert self.shortName[:2].isupper(), "Character short name doesn't start with two capital letters: %s" % self.shortName
        self.longName = self.longName.strip()

    def validate(self):
        """Validate fields other than shortName and longName."""
        assert self.dogan in self.DOGAN.itervalues(), "Bad dogan value: %s, expected %s" % (self.dogan, '/'.join(str(v) for v in self.DOGAN.itervalues()))
        assert self.isManni in (0, 1), "Bas isManni value: %d" % self.isManni
        assert not self.kaTet or all(name.isalpha() for name in self.getKaTet()), "Bad ka-tet value: %s" % self.kaTet
        assert self.nAction >= 0, "Bad nAction value: %d" % self.nAction
        assert self.hasMusic in self.HAS_MUSIC.itervalues(), "Bad hasMusic value: %s, expected %s" % (self.dogan, '/'.join(str(v) for v in self.HAS_MUSIC.itervalues()))

    def validateLinks(self):
        assert self.rID not in self.RIDS, "Duplicate character ID %s" % self.rID
        self.RIDS.add(self.rID)
        assert self.shortName.lower() not in self.SHORT_NAMES, "Duplicate character short name %r" % self.shortName
        self.SHORT_NAMES.add(self.shortName.lower())
        assert not self.longName or self.longName.lower() not in self.LONG_NAMES, "Duplicate character long name %r" % self.longName
        self.LONG_NAMES.add(self.longName.lower())
        for kaTetName in self.getKaTet():
            assert kaTetName != self.shortName, "Character is meontioned in one's own ka-tet: %s" % kaTetName
            assert kaTetName in self.CHARACTERS, "Unknown ka-tet member of %s: %s" % (self.shortName, kaTetName)
            assert self.shortName in self.CHARACTERS[kaTetName].getKaTet(), "%s is in %s's ka-tet, but %s is not in %s's one" % (kaTetName, self.shortName, self.shortName, kaTetName)

    def integrate(self):
        """Add the character to the list of characters."""
        assert CHARACTER_ID_START <= self.rID <= CHARACTER_ID_END, "Bad character ID value: %d" % self.rID
        self.CHARACTERS[self.shortName] = self

    def processFromCharactersCSV(self):
        """Process the objects after it was loaded from CSV file."""
        self.processNames()
        self.rID = int(self.rID)
        self.isNY = int(self.isNY)
        self.dogan = int(self.dogan)
        self.isManni = int(self.isManni)
        self.nAction = int(self.nAction)
        self.hasMusic = int(self.hasMusic)
        self.validate()
        self.integrate()

    def processFromJoinRPG(self):
        """Process the objects after it was loaded from JoinRPG database."""
        self.processNames()
        self.rID = None
        self.isNY = int(u'Нью-Йорк' in self.REST[0].split(' | ')) # pylint: disable=E1101
        try:
            self.dogan = self.DOGAN[self.dogan.strip()] # pylint: disable=E1101
        except KeyError:
            assert False, "%s: unknown dogan setting: %r" % (self.shortName, self.dogan)
        self.isManni = int(bool((self.isManni or '').strip()))
        self.kaTet = ':'.join(name for name in (name.strip() for name in self.SEPARATORS.split(self.kaTet)) if name)
        self.nAction = int(self.nAction.strip() or '0') # pylint: disable=E1101
        try:
            self.hasMusic = self.HAS_MUSIC[self.hasMusic]
        except KeyError:
            assert False, "%s: unknown hasMusic setting: %r" % (self.shortName, self.hasMusic)
        self.validate()

    @classmethod
    def validateAllCharacters(cls):
        """Check validity of the whole set of characters."""
        assert len(cls.CHARACTERS) <= len(CHARACTER_IDS), "Too many characters in %s file: %d" % (CHARACTERS_CSV, len(cls.CHARACTERS))
        cls.CHARACTERS = OrderedDict(sorted(cls.CHARACTERS.iteritems(), key = lambda (_shortName, character): character.rID))
        assert tuple(character.rID for character in cls.CHARACTERS.itervalues()) == tuple(xrange(CHARACTER_ID_START, CHARACTER_ID_START + len(cls.CHARACTERS))), "Damaged rIDs in %s file: %s" % (CHARACTERS_CSV, tuple(character.rID for character in cls.CHARACTERS.itervalues()))
        cls.RIDS.clear()
        cls.SHORT_NAMES.clear()
        cls.LONG_NAMES.clear()
        for character in cls.CHARACTERS.itervalues():
            character.validateLinks()

    @classmethod
    def loadCharactersCSV(cls, fileName = getFileName(CHARACTERS_CSV)):
        """Load characters from a Characters.csv file."""
        cls.CHARACTERS.clear()
        if not isfile(fileName):
            print "No character file found"
            return
        with open(fileName, 'rb') as f:
            for character in CSVObjectReader(f, cls, True, ENCODING, True):
                character.processFromCharactersCSV()
        cls.validateAllCharacters()
        print "Loaded characters: %d" % len(cls.CHARACTERS)

    @classmethod
    def saveCharactersCSV(cls, characters, fileName = getFileName(CHARACTERS_CSV), header = CHARACTERS_CSV_HEADER): # ToDo: Use CSVdumpable instead
        """Save characters to a Characters.csv file."""
        with open(fileName, 'wb') as f:
            CSVObjectWriter(f, cls, True, ENCODING, header % currentTime()).writerows(characters.itervalues())
        print "Saved characters: %d" % len(cls.CHARACTERS)

    @classmethod
    def updateFromJoinRPG(cls):
        """Update loaded characters set with data from a JoinRPG database."""
        print "Fetching data from JoinRPG.ru..."
        try:
            characters = getAllCharactersAsObjects(GAME_ID, cls, dumpCSV = True, dumpCSV1251 = True)
            print "Processing data..."
            nChanged = 0
            nAdded = 0
            for character in sorted(characters, key = lambda character: character.addSortKey()): # ToDo: sort by creation date instead, when available
                try:
                    character.processFromJoinRPG()
                except CharacterError:
                    continue
                oldCharacter = cls.CHARACTERS.get(character.shortName)
                if oldCharacter:
                    character.rID = oldCharacter.rID # makes comparison work
                    if character == oldCharacter:
                        continue
                    nChanged += 1
                else: # new character
                    character.rID = max(c.rID for c in cls.CHARACTERS.itervalues()) + 1 if cls.CHARACTERS else CHARACTER_ID_START
                    assert character.rID in CHARACTER_IDS, "Character ID range is full, can't add new character: %d" % character.rID
                    nAdded += 1
                character.integrate()
            cls.validateAllCharacters()
            if nAdded or nChanged:
                if nAdded:
                    print "Added characters: %d" % nAdded
                if nChanged:
                    print "Changed characters: %d" % nChanged
                print "Updating %s..." % CHARACTERS_CSV
                cls.saveCharactersCSV(cls.CHARACTERS)
            else:
                print "No changes detected"
        except Exception, e:
            print format_exc()
            print "ERROR fetching data, using current version: %s" % e
            return ()

    @classmethod
    def update(cls):
        """Load and update the characters set."""
        print "Processing characters..."
        cls.loadCharactersCSV()
        cls.updateFromJoinRPG()
        return cls.CHARACTERS

def updateCharacters():
    return CharacterCSVable.update()

def main():
    updateCharacters()

if __name__ == '__main__':
    main()

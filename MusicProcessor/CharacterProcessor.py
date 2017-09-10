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

from CSVable import CSVable, CSVObjectReader, CSVObjectWriter
from joinrpg import getAllCharacters

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
        ( 'characterID', 'characterID'),
        (u'Имя на браслете', 'shortName'),
        (u'Имя персонажа', 'longName'),
        (u'Мэнни?', 'isManni'),
        (u'Нью-Йорк?', 'isNY'),
        (u'Внутренний Доган', 'dogan'),
        (u'Связи ка-тета', 'kaTet'),
        (u'Стартовое число очков действия', 'nAction'),
        (u'Музыка прислана?', 'hasMusic')
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

    def getKaTet(self):
        return tuple(self.kaTet.split(self.KA_TET_SEP)) if self.kaTet else ()

    def setKaTet(self, kaCharacters):
        self.kaTet = self.KA_TET_SEP.join(kaCharacters)

    def processNames(self):
        """Process and validate shortName and longName."""
        self.shortName = (self.shortName or '').strip()
        if not self.shortName:
            raise CharacterError("Character short name is empty")
        try:
            self.shortName = self.shortName.encode('ascii')
        except UnicodeError:
            assert False, "Character short name is not ASCII: %r" % self.shortName
        assert self.shortName.isalpha(), "Character short name is not alphabetic: %s" % self.shortName
        assert self.shortName[:2].isupper(), "Character short name doesn't start with two capital letters: %s" % self.shortName
        self.longName = (self.longName or '').strip()

    def validate(self):
        """Validate fields other than shortName and longName."""
        assert self.isManni in (0, 1), "Bas isManni value: %d" % self.isManni
        assert self.isNY in (0, 1), "Bas isNY value: %d" % self.isNY
        assert self.dogan in self.DOGAN.itervalues(), "Bad dogan value: %s, expected %s" % (self.dogan, '/'.join(str(v) for v in self.DOGAN.itervalues()))
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
            assert set(self.CHARACTERS[kaTetName].getKaTet() + (self.CHARACTERS[kaTetName].shortName,)) == set(self.getKaTet() + (self.shortName,)), "Ka-tets for %s and %s do not match" % (self.shortName, kaTetName)

    def integrate(self):
        """Add the character to the list of characters."""
        assert CHARACTER_ID_START <= self.rID <= CHARACTER_ID_END, "Bad character ID value: %d" % self.rID
        self.CHARACTERS[self.shortName] = self

    def processFromCharactersCSV(self):
        """Process the object after it was loaded from CSV file."""
        self.processNames()
        self.rID = int(self.rID)
        self.characterID = int(self.characterID)
        self.isManni = int(self.isManni)
        self.isNY = int(self.isNY)
        self.dogan = int(self.dogan)
        self.nAction = int(self.nAction)
        self.hasMusic = int(self.hasMusic)
        self.validate()
        self.integrate()

    def fromJoinRPG(self, jCharacter):
        """Construct the object from informated loaded from JoinRPG."""
        for (name, field) in self.CSV_FIELDS.iteritems():
            try:
                setattr(self, field, getattr(jCharacter, field))
            except AttributeError:
                try:
                    setattr(self, field, jCharacter.fieldValues[name])
                except KeyError:
                    pass
        self.characterID = jCharacter.characterId
        self.processNames()
        self.rID = None
        self.isManni = int(bool((self.isManni or '').strip()))
        self.isNY = int(u'Нью-Йорк' in jCharacter.groupNames)
        try:
            self.dogan = self.DOGAN[self.dogan.strip()] # pylint: disable=E1101
        except KeyError:
            assert False, "%s: unknown dogan setting: %r" % (self.shortName, self.dogan)
        self.kaTet = ':'.join(name for name in (name.strip() for name in self.SEPARATORS.split(self.kaTet or '')) if name)
        self.nAction = int((self.nAction or '').strip() or '0') # pylint: disable=E1101
        try:
            self.hasMusic = self.HAS_MUSIC[self.hasMusic or '']
        except KeyError:
            assert False, "%s: unknown hasMusic setting: %r" % (self.shortName, self.hasMusic)
        self.validate()
        return self

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
        print "Loading characters..."
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
            jCharacters = getAllCharacters(GAME_ID, cacheData = True, cacheAuth = True)
            print "Loaded %d character entities" % len(jCharacters)
            nLoaded = nChanged = nAdded = nSkipped = 0
            for jCharacter in jCharacters:
                try:
                    character = CharacterCSVable()
                    character.fromJoinRPG(jCharacter)
                except CharacterError, e:
                    nSkipped += 1
                    continue
                nLoaded += 1
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
            if nLoaded:
                print "Loaded %d valid characters" % nLoaded
            else:
                print "No valid characters found"
            if nSkipped:
                print "Skipped %d characters" % nSkipped
            if nAdded or nChanged:
                if nAdded:
                    print "Added %d characters" % nAdded
                if nChanged:
                    print "Changed %d characters" % nChanged
                print "Updating %s..." % CHARACTERS_CSV
                cls.saveCharactersCSV(cls.CHARACTERS)
            else:
                print "No changes detected"
        except Exception, e:
            #print format_exc()
            print "ERROR fetching data: %s, using cached version" % unicode(e)
            return ()

    @classmethod
    def update(cls):
        """Load and update the characters set."""
        cls.loadCharactersCSV()
        cls.updateFromJoinRPG()
        return cls.CHARACTERS

def updateCharacters():
    return CharacterCSVable.update()

def main():
    updateCharacters()

if __name__ == '__main__':
    main()

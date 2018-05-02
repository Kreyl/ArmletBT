#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Character Processor for "Dark Tower: All Hail" LARP.
#
# - Downloads character data from JoinRPG project.
# - Loads, verifies and updates Characters.csv file
#
# Usage: python CharacterProcessor.py
#
from collections import OrderedDict
from re import compile as reCompile
from traceback import format_exc

from Settings import CHARACTER_ID_START, CHARACTER_ID_END, CHARACTER_IDS
from Structures import CSVdumpable

from joinrpg import getAllCharacters

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

class CharacterCSVable(CSVdumpable):
    CSV_FIELDS = ('rID', 'shortName', 'isNY', 'isManni', 'dogan', 'kaTet', 'nAction', 'isDead', 'hasMusic')

    KEY_FUNCTION = lambda character: character.shortName

    JOINRPG_FIELDS = dict((
            ('shortName', u'Имя на браслете'),
            ('isManni', u'Мэнни?'),
            ('dogan', u'Внутренний Доган'),
            ('kaTet', u'Связи ка-тета'),
            ('nAction', u'Стартовое число очков действия'),
            ('isDead', u'Мёртв?'),
            ('hasMusic', u'Музыка прислана?')))

    DOGAN = OrderedDict(((u'Служба Алому Королю', -2), (u'Алое Колебание', -1), (u'Нейтралитет', 0), (u'Белое Колебание', 1), (u'Путь Белизны', 2)))

    NO = 0
    UNCHECKED = 1
    CHECKED = 2
    HAS_MUSIC = OrderedDict((('', NO), (u'да, не проверена', UNCHECKED), (u'да, проверена', CHECKED)))

    SEPARATORS = reCompile('[,; ]*')

    KA_TET_SEP = ':'

    INSTANCES = OrderedDict()
    TITLE = 'Characters'
    HEADER_TITLE = '''# Characters table for ArmLet initialization.
#
# Generated, updated from JoinRPG and used by CharacterProcessor.py
# to track persistent unique character Reason IDs.'''

    RIDS = set()
    SHORT_NAMES = set()

    def getKaTet(self):
        return set(self.kaTet.split(self.KA_TET_SEP)) if self.kaTet else set()

    def setKaTet(self, kaCharacters):
        self.kaTet = self.KA_TET_SEP.join(sorted(kaCharacters))

    def processNames(self):
        """Process and validate shortName."""
        self.shortName = (self.shortName or '').strip()
        if not self.shortName:
            raise CharacterError("Character short name is empty")
        try:
            self.shortName = self.shortName.encode('ascii')
        except UnicodeError:
            assert False, "Character short name is not ASCII: %r" % self.shortName
        assert self.shortName.isalnum(), "Character short name is not alphanumeric: %s" % self.shortName
        assert self.shortName[:1].isupper(), "Character short name doesn't start with a capital letter: %s" % self.shortName

    def validate(self):
        """Validate fields other than shortName."""
        assert self.isManni in (0, 1), "Bas isManni value: %d" % self.isManni
        assert self.isNY in (0, 1), "Bas isNY value: %d" % self.isNY
        assert self.dogan in self.DOGAN.itervalues(), "Bad dogan value: %s, expected %s" % (self.dogan, '/'.join(str(v) for v in self.DOGAN.itervalues()))
        assert not self.kaTet or all(name.isalpha() for name in self.getKaTet()), "Bad ka-tet value: %s" % self.kaTet
        assert self.nAction >= 0, "Bad nAction value: %d" % self.nAction
        assert self.isDead in (0, 1), "Bas isDead value: %d" % self.isDead
        assert self.hasMusic in self.HAS_MUSIC.itervalues(), "Bad hasMusic value: %s, expected %s" % (self.dogan, '/'.join(str(v) for v in self.HAS_MUSIC.itervalues()))

    def validateLinks(self):
        assert self.rID not in self.RIDS, "Duplicate character ID %s" % self.rID
        self.RIDS.add(self.rID)
        assert self.shortName.lower() not in self.SHORT_NAMES, "Duplicate character short name %r" % self.shortName
        self.SHORT_NAMES.add(self.shortName.lower())
        kaTet = self.getKaTet()
        if kaTet:
            assert not kaTet or self.shortName in kaTet, "Character is not in one's own ka-tet: %s" % self.shortName
            assert len(kaTet) > 1, "Character is the only member in one's ka-tet: %s" % self.shortName
            for kaTetName in kaTet:
                assert kaTetName in self.INSTANCES, "Unknown ka-tet member of %s: %s" % (self.shortName, kaTetName)
                assert kaTet == self.INSTANCES[kaTetName].getKaTet(), "Ka-tets for %s and %s are different" % (self.shortName, kaTetName)

    def integrate(self):
        """Add the character to the list of characters."""
        assert CHARACTER_ID_START <= self.rID <= CHARACTER_ID_END, "Bad character ID value: %d" % self.rID
        self.INSTANCES[self.shortName] = self

    def processFromCharactersCSV(self):
        """Process the object after it was loaded from CSV file."""
        self.processNames()
        self.rID = int(self.rID)
        self.isManni = int(self.isManni)
        self.isNY = int(self.isNY)
        self.dogan = int(self.dogan)
        self.nAction = int(self.nAction)
        self.isDead = int(self.isDead)
        self.hasMusic = int(self.hasMusic)
        self.validate()
        self.integrate()

    def fromJoinRPG(self, jCharacter):
        """Construct the object from informated loaded from JoinRPG."""
        for field in self._getFields():
            try:
                setattr(self, field, getattr(jCharacter, field))
            except AttributeError:
                try:
                    setattr(self, field, jCharacter.fieldValues[self.JOINRPG_FIELDS[field]])
                except KeyError:
                    pass
        self.processNames()
        self.rID = None
        self.isManni = int(bool((self.isManni or '').strip()))
        self.isDead = int(bool((self.isDead or '').strip()))
        self.isNY = int(u'Нью-Йорк' in jCharacter.groupNames)
        try:
            self.dogan = self.DOGAN[self.dogan.strip()] # pylint: disable=E1101
        except KeyError:
            assert False, "%s: unknown dogan setting: %r" % (self.shortName, self.dogan)
        self.setKaTet(name for name in (name.strip() for name in self.SEPARATORS.split(self.kaTet or '')) if name)
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
        assert len(cls.INSTANCES) <= len(CHARACTER_IDS), "Too many characters in %s file: %d" % (CHARACTERS_CSV, len(cls.INSTANCES))
        cls.INSTANCES = OrderedDict(sorted(cls.INSTANCES.iteritems(), key = lambda (_shortName, character): character.rID))
        assert tuple(character.rID for character in cls.INSTANCES.itervalues()) == tuple(xrange(CHARACTER_ID_START, CHARACTER_ID_START + len(cls.INSTANCES))), "Damaged rIDs in %s file: %s" % (CHARACTERS_CSV, tuple(character.rID for character in cls.INSTANCES.itervalues()))
        cls.RIDS.clear()
        cls.SHORT_NAMES.clear()
        for character in cls.INSTANCES.itervalues():
            character.validateLinks()
        for character in cls.INSTANCES.itervalues():
            character.kaTetIDs = ','.join(sorted(str(cls.INSTANCES[k].rID) for k in character.getKaTet() if k != character.shortName))

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
                    character = CharacterCSVable().fromJoinRPG(jCharacter)
                except CharacterError, e:
                    nSkipped += 1
                    continue
                nLoaded += 1
                oldCharacter = cls.INSTANCES.get(character.shortName)
                if oldCharacter:
                    character.rID = oldCharacter.rID # makes comparison work
                    if character == oldCharacter:
                        continue
                    nChanged += 1
                else: # new character
                    character.rID = max(c.rID for c in cls.INSTANCES.itervalues()) + 1 if cls.INSTANCES else CHARACTER_ID_START
                    assert character.rID in CHARACTER_IDS, "Character ID range is full, can't add new character: %d" % character.rID
                    nAdded += 1
                character.integrate()
            cls.validateAllCharacters()
            if nSkipped:
                print "Skipped %d characters" % nSkipped
            if nLoaded:
                print "Loaded %d valid characters" % nLoaded
            else:
                print "No valid characters found"
            if nAdded or nChanged:
                if nAdded:
                    print "Added %d characters" % nAdded
                if nChanged:
                    print "Changed %d characters" % nChanged
                print "Updating %s..." % CHARACTERS_CSV
                cls.dumpCSV()
                print "Saved characters: %d" % len(cls.INSTANCES)
            else:
                print "No changes detected"
        except Exception, e:
            print format_exc()
            print "ERROR fetching data: %s, using cached version" % unicode(e)

    @classmethod
    def update(cls):
        """Load and update the characters set."""
        print "Loading characters..."
        cls.INSTANCES.clear()
        cls.loadCSV()
        for character in cls.INSTANCES.itervalues():
            character.processFromCharactersCSV()
        cls.validateAllCharacters()
        print "Loaded characters: %d" % len(cls.INSTANCES)
        cls.updateFromJoinRPG()
        return cls.INSTANCES

def updateCharacters():
    print
    print "Running DTAH CharacterProcessor"
    print
    return CharacterCSVable.update()

def main():
    updateCharacters()

if __name__ == '__main__':
    main()

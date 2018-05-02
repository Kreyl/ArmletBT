#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Emotions Processor for "Dark Tower: All Hail" LARP.
#
# Usage:
# - Downloads basic reasons and emotions data from Google Docs.
# - Also processes Characters.csv and produces Emotions.csv, Reasons.csv, emotions.c and emotions.h.
# - Compile and link emotions.c and emotions.h with whatever code that needs emotions data.
#
from collections import OrderedDict
from os.path import join
from platform import system
from re import compile as reCompile
from subprocess import Popen, PIPE, STDOUT
from traceback import format_exc
from urllib2 import urlopen

from CSVable import CSVable, CSVObjectReader
from CharacterProcessor import updateCharacters
from Settings import getFileName, REASON_ID_START, REASON_ID_END
from Structures import Reason, Emotion

isWindows = system().lower().startswith('win')

ENCODING = 'windows-1251'

GOOGLE_DOC_ID_FILE_NAME = 'musicGoogleDoc.id'
CSV_URL = 'https://docs.google.com/spreadsheets/d/%s/export?format=csv'
DUMP_FILE_NAME = 'musicGoogleDoc.csv'
DUMP_FILE_NAME_1251 = 'musicGoogleDoc-1251.csv'

C_PATH = '../Armlet_fw/SharedLogic'

C_TARGET = join(C_PATH, 'emotions.cpp')

H_TARGET = join(C_PATH, 'emotions.h')

TEST_COMMAND = 'gcc -I "%s" -o test "%s" test.c && ./test && rm test' % (C_PATH, C_TARGET)

class GoogleTableEntry(CSVable):
    CSV_FIELDS = ('rID', 'rName', 'level', 'timeout', 'doganAmount', 'eName', 'eType', 'ePriority', 'contents')

    REASON_PATTERN = reCompile(r'[A-Z][A-Z0-9_]*|[A-Z][a-zA-Z]*')
    EMOTION_PATTERN = reCompile(r'[A-Z][A-Z0-9_]*')

    LEVELS = OrderedDict({'NONE': 0, 'NEAR': 1, 'MEDIUM': 2, 'FAR': 3})
    ETYPES = OrderedDict({'SINGLE': 0, 'REPEAT': 1})

    def processFromGoogleCSV(self):
        """Process the objects after it was loaded from CSV exported from Google Docs spreadsheet."""
        # rName
        self.rName = self.rName.strip()
        if not self.rName:
            return
        try:
            self.rID = int(self.rID)
        except ValueError:
            assert False, "rID is not a number for reason %s: %r" % (self.rName, self.rID)
        assert REASON_ID_START <= self.rID <= REASON_ID_END, "Incorrect rID for reason %s: %r, expected %s <= rID < %d" % (self.rName, self.rID, REASON_ID_START, REASON_ID_END)
        assert self.rID not in (r.rID for r in Reason.INSTANCES.itervalues()), "Duplicate rID: %s" % self.rID
        try:
            self.rName = self.rName.encode('ascii')
        except UnicodeError:
            assert False, "Reason name is not ASCII: %r" % self.rName
        if self.rName != '-':
            assert self.REASON_PATTERN.match(self.rName), "Reason name is not in PROPER_FORMAT_WITH_DIGITS: %s" % self.rName
        assert self.rName not in Reason.INSTANCES, "Duplicate reason name: %s" % self.rName
        # level
        try:
            self.level = self.LEVELS[self.level.upper()]
        except KeyError:
            assert False, "Incorrect level for reason %s: %r, expected '%s'" % (self.rName, self.level, '\' or \''.join(self.LEVELS))
        # timeout
        try:
            self.timeout = int(self.timeout or 0)
        except ValueError:
            assert False, "Timeout is not a number for reason %s: %r" % (self.rName, self.timeout)
        assert 0 <= self.timeout <= 999 or self.timeout == -1, "Bad timeout for reason %s: %r" % (self.rName, self.timeout)
        # doganAmount
        try:
            self.doganAmount = int(self.doganAmount or 0)
        except ValueError:
            assert False, "Dogan amount is not a number for reason %s: %r" % (self.rName, self.doganAmount)
        assert -3 <= self.doganAmount <= 5, "Bad dogan amount for reason %s: %r" % (self.rName, self.doganAmount)
        # eName
        try:
            self.eName = self.eName.strip().encode('ascii')
        except UnicodeError:
            assert False, "Emotion name is not ASCII for reason %s: %r" % (self.rName, self.eName)
        assert not self.eName or self.EMOTION_PATTERN.match(self.eName), "Emotion name is not in PROPER_FORMAT_WITH_DIGITS for reason %s: %s" % (self.rName, self.eName)
        if self.eName:
            # eType
            try:
                self.eType = self.ETYPES[self.eType.upper()]
            except KeyError:
                assert False, "Incorrect eType for reason %s: %r, expected '%s'" % (self.rName, self.eType, '\' or \''.join(self.ETYPES))
            # ePriority
            try:
                self.ePriority = int(self.ePriority or 0)
            except ValueError:
                assert False, "Priority is not a number for reason %s, emotion %s: %r" % (self.rName, self.eName, self.ePriority)
        # isPlayer
        self.isPlayer = int(u'игроки' in self.contents) # pylint: disable=E1101
        # ePriority consistency
        emotion = Emotion.INSTANCES.get(self.eName)
        if emotion:
            assert emotion.eType == self.eType, "Non-consistent type for emotion %s: %d and %d" % (self.eName, emotion.eType, self.eType)
            assert emotion.ePriority == self.ePriority, "Non-consistent priority for emotion %s: %d and %d" % (self.eName, emotion.ePriority, self.ePriority)
        elif self.eName:
            emotion = Emotion.addEmotion(Emotion(self.eName, self.eType, self.ePriority, self.isPlayer))
        else:
            emotion = None
        Reason.addReason(Reason(self.rID, self.rName, self.level, self.timeout, self.doganAmount, self.eName))

    @classmethod
    def loadFromGoogleDocs(cls, dumpCSV = False, dumpCSV1251 = False):
        """Loads the reasons and emotions from the Google Docs spreadsheet."""
        print "Fetching data from Google Docs..."
        Reason.INSTANCES.clear()
        Emotion.INSTANCES.clear()
        for attempt in xrange(1, 3):
            try:
                if attempt == 1: # load from URL
                    googleDocID = open(getFileName(GOOGLE_DOC_ID_FILE_NAME)).read().strip()
                    url = CSV_URL % googleDocID
                    data = urlopen(url).read()
                else: # attempt == 2: # load from cached copy
                    data = open(getFileName(DUMP_FILE_NAME), 'rb').read()
                for self in CSVObjectReader(data.splitlines(), cls, True, 'utf-8'):
                    self.processFromGoogleCSV()
                break
            except Exception, e:
                if attempt == 1:
                    print format_exc()
                    print "ERROR fetching data: %s, using cached version" % e
                else:
                    raise
        if attempt == 1: # pylint: disable=W0631
            if dumpCSV:
                with open(dumpCSV if dumpCSV is not True else getFileName(DUMP_FILE_NAME), 'wb') as f:
                    f.write(data)
            if dumpCSV1251:
                with open(dumpCSV1251 if dumpCSV1251 is not True else getFileName(DUMP_FILE_NAME_1251), 'wb') as f:
                    f.write(data.decode('utf-8').encode(ENCODING))

    @classmethod
    def processEmotions(cls):
        Reason.addCharacters(cls.CHARACTERS.itervalues())
        Reason.sortByIDs()
        for (eID, emotion) in enumerate(Emotion.sort().itervalues()):
            emotion.eID = eID
        Reason.dumpCSV()
        print "Reasons dumped: %d" % len(Reason.INSTANCES)
        Reason.dumpCPP()
        print "Reasons CPP code dumped: %d" % len(Reason.INSTANCES)
        Emotion.dumpCSV()
        print "Emotions dumped: %d" % len(Emotion.INSTANCES)

    @classmethod
    def update(cls):
        """Update characters, reasons and emotions."""
        cls.CHARACTERS = updateCharacters()
        cls.loadFromGoogleDocs(True, True)
        print "Processing emotions..."
        cls.processEmotions()
        if True: #isWindows: # pylint: disable=W0125
            print "Not running test on Windows\nDone"
        else:
            print "Running test: %s" % TEST_COMMAND
            subprocess = Popen(TEST_COMMAND, shell = True, stdout = PIPE, stderr = STDOUT)
            output = subprocess.communicate()[0]
            print "Done (%s): %s" % (subprocess.returncode, output),
        return (Emotion.INSTANCES, cls.CHARACTERS)

def updateEmotions():
    return GoogleTableEntry.update()

def main():
    updateEmotions()

if __name__ == '__main__':
    main()

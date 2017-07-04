#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Emotions Processor for "Dark Tower: All Hail" LARP.
#
# Usage:
# - Downloads basic reasons and emotions data from Google Docs.
# - Also processes Locations.csv and Characters.csv
#   and produces Emotions.csv, Reasons.csv, emotions.c and emotions.h.
# - Compile and link emotions.c and emotions.h with whatever code that needs emotions data.
#
from os.path import join
from platform import system
from re import compile as reCompile
from subprocess import Popen, PIPE, STDOUT
#from traceback import format_exc
from urllib2 import urlopen

from CSVable import CSVable, CSVObjectReader
from CharacterProcessor import updateCharacters
from Settings import getFileName, SOURCE_ID_START
from Structures import Source, Reason, Emotion

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
    CSV_FIELDS = ('rName', 'rPriority', 'nSources', 'eName', 'ePriority')

    REASON_PATTERN = reCompile(r'[A-Z][A-Z0-9_]*|[A-Z][a-zA-Z]*')
    EMOTION_PATTERN = reCompile(r'[A-Z][A-Z0-9_]*')

    def processFromGoogleCSV(self):
        """Process the objects after it was loaded from CSV exported from Google Docs spreadsheet."""
        # rName
        self.rName = self.rName.strip()
        if not self.rName:
            return
        try:
            self.rName = self.rName.encode('ascii')
        except UnicodeError:
            assert False, "Reason name is not ASCII: %r" % self.rName
        if self.rName != '-':
            assert self.REASON_PATTERN.match(self.rName), "Reason name is not in PROPER_FORMAT_WITH_DIGITS: %s" % self.rName
        assert self.rName not in Reason.INSTANCES, "Duplicate reason name: %s" % self.rName
        # rPriority
        self.rPriority = self.rPriority.strip() # pylint: disable=E1101
        assert self.rPriority, "Priority not specified for reason %s" % self.rName
        try:
            self.rPriority = int(self.rPriority)
        except ValueError:
            assert False, "Priority is not a number for reason %s: %r" % (self.rName, self.rPriority)
        # nSources
        try:
            self.nSources = int(self.nSources)
        except ValueError:
            assert False, "Number of sources is not a number for reason %s: %r" % (self.rName, self.nSources)
        assert 0 <= self.nSources <= 30, "Bad number of sources for reason %s: %r" % (self.rName, self.nSources)
        # eName
        try:
            self.eName = self.eName.strip().encode('ascii')
        except UnicodeError:
            assert False, "Emotion name is not ASCII for reason %s: %r" % (self.rName, self.eName)
        assert self.EMOTION_PATTERN.match(self.eName), "Emotion name is not in PROPER_FORMAT_WITH_DIGITS for reason %s: %s" % (self.rName, self.eName)
        # ePriority
        try:
            self.ePriority = int(self.ePriority) if self.ePriority else None
        except ValueError:
            assert False, "Priority is not a number for reason %s, emotion %s: %r" % (self.rName, self.eName, self.ePriority)
        # ePriority consistency
        emotion = Emotion.INSTANCES.get(self.eName)
        if emotion:
            if emotion.ePriority is None:
                if self.ePriority is not None:
                    emotion.ePriority = self.ePriority
            elif self.ePriority is not None:
                assert emotion.ePriority == self.ePriority, "Non-consistent priority for emotion %s: %d and %d" % (self.eName, emotion.ePriority, self.ePriority)
        else:
        # Fill in the tables
            emotion = Emotion.addEmotion(Emotion(self.eName, self.ePriority))
        if self.rName != '-':
            Reason.addReason(Reason(self.rName, self.rPriority, self.nSources, self.eName))

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
                    #print format_exc()
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
        Reason.addPlaceholders()
        Reason.sort()
        rID = 0
        for reason in Reason.INSTANCES.itervalues():
            if reason.rID is None:
                reason.rID = rID
                rID += 1
        Reason.sortByIDs()
        assert tuple(reason.rID for reason in Reason.INSTANCES.itervalues()) == tuple(xrange(len(Reason.INSTANCES))), "Damaged rIDs table: %s" % Reason.INSTANCES
        nSources = SOURCE_ID_START
        Source.INSTANCES[:] = []
        for reason in Reason.INSTANCES.itervalues():
            newNSources = nSources + reason.nSources
            for sID in xrange(nSources, newNSources):
                Source.INSTANCES.append(Source(sID, reason.rName))
            nSources = newNSources
        for (eID, emotion) in enumerate(Emotion.sort().itervalues()):
            emotion.eID = eID
        assert len(Source.INSTANCES) == nSources - SOURCE_ID_START, "Reason table length mismatch: %d, expected %d" % (len(Source.INSTANCES), nSources)
        assert tuple(source.sID for source in Source.INSTANCES) == tuple(xrange(SOURCE_ID_START, nSources)), "Damaged sIDs table: %s" % Source.INSTANCES
        Source.dumpCSV()
        print "Sources dumped: %d" % len(Source.INSTANCES)
        Reason.dumpCSV()
        print "Reasons dumped: %d" % len(Reason.INSTANCES)
        Emotion.dumpCSV()
        print "Emotions dumped: %d" % len(Emotion.INSTANCES)

    @classmethod
    def update(cls):
        """Update characters, reasons and emotions."""
        cls.CHARACTERS = updateCharacters()
        cls.loadFromGoogleDocs(True, True)
        print "Processing emotions..."
        cls.processEmotions()
        #cls.writeC()
        #cls.writeH()
        if True: #isWindows: # pylint: disable=W0125
            print "Not running test on Windows\nDone"
        else:
            print "Running test: %s" % TEST_COMMAND
            subprocess = Popen(TEST_COMMAND, shell = True, stdout = PIPE, stderr = STDOUT)
            output = subprocess.communicate()[0]
            print "Done (%s): %s" % (subprocess.returncode, output),
        return Reason.INSTANCES

def updateEmotions():
    return GoogleTableEntry.update()

def main():
    updateEmotions()

if __name__ == '__main__':
    main()

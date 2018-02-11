#!/usr/bin/python
#
# Music Processor for "Dark Tower: All Hail" LARP.
#
# - Uses CharacterProcessor.py to update character information.
# - Uses EmotionProcessor.py to update emotions information.
# - Updates and verifies information in DTAH Music Directory
#   specified with DTAH_MUSIC environment variable, with a parameter
#   or in current directory.
# - DTAH Music directory must contain DTAH_Music_Here file.
# - If -v|--verify option is used, actual processed files are verified for consistency.
#
# Usage:
# - python MusicProcessor.py [-v|--verify] [musicDirectory]
#
from codecs import open as codecsOpen
from datetime import datetime
from errno import ENOENT
from getopt import getopt
from itertools import chain, count
from os import getenv, listdir, makedirs, remove, rmdir, walk
from os.path import basename, expanduser, getmtime, getsize, isdir, isfile, islink, join
from re import compile as reCompile
from shutil import copy, copytree, rmtree
from sys import argv, platform, stdout
from time import mktime
from traceback import format_exc

try:
    from pydub import AudioSegment
except ImportError, ex:
    raise ImportError("%s: %s\n\nPlease install pydub v0.20 or later: https://pypi.python.org/pypi/pydub\n" % (ex.__class__.__name__, ex))

from CharacterProcessor import CharacterCSVable

from EmotionConverter import convert, convertEmotion, convertTitle
from EmotionProcessor import updateEmotions

#
# ToDo: Make sure personal music doesn't contain (warn about them) common track names like smert', tuman etc.
#

MUSIC_LOCATION_VARIABLE = 'DTAH_MUSIC'

SOURCE_DIR = 'src'
ARMLET_DIR = 'armlet'
MUSIC_DIR = 'music'
ERROR_DIR = 'errors'

SD_DIR = '_SD'
COMMON_DIR = '_COMMON'
EXCLUDE_DIRS = (SD_DIR, COMMON_DIR)

INI_FILE = 'settings.ini'

INI_CONTENT = open('settings_ini.tpl').read().replace('\r\n', '\n').replace('\n', '\r\n')

CHARACTER_CSV_PATTERN = reCompile(r'character.*\.csv')

FILE_PATTERN = reCompile(r'.*\..*')

EMOTION = 'emotion'
ARTIST = 'artist'
TITLE = 'title'
TAIL = 'tail'

SEPARATOR = '-'

CHECK_PATTERN = reCompile(r'(?i)^(?P<%s>[^%s\s\d]+)\s*\d*\s*%s\s*(?P<%s>[^%s]*)(?:\s*%s\s*(?P<%s>[^%s]*?)(?:\s*%s\s*(?P<%s>.*))?)?\.[^.]*$' % (EMOTION, SEPARATOR, SEPARATOR, ARTIST, SEPARATOR, SEPARATOR, TITLE, SEPARATOR, SEPARATOR, TAIL))
NEW_FORMAT = 'wav'

MAX_FILE_NAME = 64

NEW_EXTENSION = '.%s' % NEW_FORMAT

DEFAULT_TARGET_DIR = 'processed'

MUSIC_MARK = 'DTAH_Music_Here'

RESULT_MARKS = { True: 'music_errors', False: 'music_ok' }

INVALID_FILENAME_CHARS = '<>:"/\\|?*' # for file names, to be replaced with _
def cleanupFileName(fileName):
    return ''.join('_' if c in INVALID_FILENAME_CHARS else c for c in fileName)

isWindows = platform.lower().startswith('win')
CONSOLE_ENCODING = stdout.encoding or ('cp866' if isWindows else 'UTF-8')
def encodeForConsole(s):
    return s.encode(CONSOLE_ENCODING, 'replace')

def getFileModificationTime(fileName):
    if not isfile(fileName):
        return None
    t = getmtime(fileName)
    dt = datetime.fromtimestamp(t)
    if dt.year > 2050:
        return mktime(dt.replace(year = dt.year - 100).timetuple()) # sorry, no easier way to do it
    return t

def silentRemove(filename):
    try:
        remove(filename)
    except OSError, e:
        if e.errno != ENOENT:
            raise

def createDir(dirName):
    if not isdir(dirName):
        makedirs(dirName)

def deepListDir(dirName):
    return tuple(chain.from_iterable(((dirPath, fileName) for fileName in fileNames) for (dirPath, dirNames, fileNames) in walk(dirName)))

def getFiles(dirName):
    return tuple(join(dirName, f) for f in listdir(dirName)) if isdir(dirName) else ()

def deepGetFiles(dirName):
    return tuple(join(d, f) for (d, f) in deepListDir(dirName)) if isdir(dirName) else ()

def processFile(fullName, newFullName):
    try:
        sourceAudio = AudioSegment.from_file(fullName)
        if sourceAudio.duration_seconds < 4:
            return "Audio too short: %d seconds" % sourceAudio.duration_seconds
        if sourceAudio.duration_seconds < 60:
            print "\nWARNING: %s: Audio too short: %d seconds" % (encodeForConsole(basename(fullName)), sourceAudio.duration_seconds)
        processedAudio = sourceAudio.normalize() # pylint: disable=E1103
        processedAudio.set_sample_width(2)
        processedAudio.set_frame_rate(44100)
        processedAudio.set_channels(2)
        if processedAudio.duration_seconds != sourceAudio.duration_seconds:
            return "Normalized audio duration mismatch: %d seconds, expected %d seconds" % (processedAudio.duration_seconds, sourceAudio.duration_seconds)
        processedAudio.export(newFullName, format = NEW_FORMAT)
        if not isfile(newFullName) or getsize(newFullName) < 0.1 * getsize(fullName):
            return "Processed file is too small: %d bytes, while original file was %d bytes" % (getsize(newFullName), getsize(fullName))
        return None
    except Exception, e:
        print format_exc()
        return e

def verifyFile(fullName):
    try:
        sourceAudio = AudioSegment.from_file(fullName)
        if sourceAudio.duration_seconds < 4:
            return "Audio too short: %d seconds" % sourceAudio.duration_seconds
        if sourceAudio.duration_seconds < 60:
            print "\nWARNING: %s: Audio too short: %d seconds" % (encodeForConsole(basename(fullName)), sourceAudio.duration_seconds)
        processedAudio = sourceAudio.normalize() # pylint: disable=E1103
        if processedAudio.duration_seconds != sourceAudio.duration_seconds:
            return "Normalized audio duration mismatch: %d seconds, expected %d seconds" % (processedAudio.duration_seconds, sourceAudio.duration_seconds)
        processedAudio.export(format = 'null')
        return None
    except Exception, e:
        return e

def resultMark(targetDir, result, okNum = None, okSize = None, errorText = None):
    for markName in RESULT_MARKS.itervalues():
        silentRemove(join(targetDir, markName))
    if okNum is not None:
        with open(join(targetDir, RESULT_MARKS[False]), 'wb') as f:
            f.write('%d,%d\r\n' % (okNum, okSize))
    if result:
        with codecsOpen(join(targetDir, RESULT_MARKS[True]), 'wb', 'utf-8') as f:
            f.write(errorText)

def checkResultMark(targetDir):
    okMark = join(targetDir, RESULT_MARKS[False])
    okDate = getFileModificationTime(okMark) if isfile(okMark) else 0
    okText = ''
    if okDate:
        with open(okMark) as f:
            okText = tuple(int(i) for i in f.read().split(','))
            okNum = okText[0]
            okSize = okText[1] if len(okText) > 1 else 0
    else:
        okNum = okSize = None
    errorMark = join(targetDir, RESULT_MARKS[True])
    errorDate = getFileModificationTime(errorMark) if isfile(errorMark) else 0
    errorText = ''
    if errorDate:
        with codecsOpen(errorMark, 'r', 'utf-8') as f:
            errorText = f.read()
    return (bool(errorDate), max(okDate, errorDate), okNum, okSize, errorText)

def processCharacter(character, emotions, baseDir = '.', verifyFiles = False):
    class ProcessException(Exception):
        pass
    def log(error, fileName, message):
        s = '%s%s' % (('%s: ' % fileName) if fileName else '', message)
        if fileName:
            print
        print encodeForConsole(s)
        messages.append('%s\r\n' % s)
        hasErrors[0] = hasErrors[0] or error # pylint: disable=E0601
    print "\nProcessing character: %s (%s)" % (character.shortName, character.rID if character.rID is not None else 'UNKNOWN')
    messages = []
    hasErrors = [False]
    sdDir = join(unicode(baseDir), SD_DIR)
    commonDir = join(unicode(baseDir), COMMON_DIR)
    baseDir = join(unicode(baseDir), character.shortName)
    sourceDir = join(baseDir, SOURCE_DIR)
    errorDir = join(baseDir, ERROR_DIR)
    armletDir = join(baseDir, ARMLET_DIR)
    musicDir = join(armletDir, MUSIC_DIR)
    createDir(armletDir)
    sourceFiles = deepGetFiles(sourceDir)
    musicFiles = getFiles(musicDir)
    errorFiles = getFiles(errorDir)
    newFileNameSet = set()
    # Removing common files
    for fileName in (join(armletDir, f) for f in listdir(armletDir) if f != MUSIC_DIR and not CHARACTER_CSV_PATTERN.match(f)):
        if isdir(fileName) and not islink(fileName):
            rmtree(fileName)
        else:
            remove(fileName)
    # Copying SD files
    for fileName in listdir(sdDir):
        src = join(sdDir, fileName)
        dst = join(armletDir, fileName)
        if isdir(src):
            copytree(src, dst)
        else:
            copy(src, dst)
    # Copying common files
    for fileName in listdir(commonDir):
        src = join(commonDir, fileName)
        dst = join(armletDir, 'common', fileName)
        if isdir(src):
            copytree(src, dst)
        else:
            copy(src, dst)
    # Creating settings.ini
    if character.rID > 0:
        with open(join(armletDir, INI_FILE), 'wb') as f:
            fields = character.getFields()
            fields['kaTetIDs'] = character.kaTetIDs
            f.write(INI_CONTENT % fields)
    # Processing character.csv
    characterFiles = tuple(fileName for fileName in listdir(armletDir) if CHARACTER_CSV_PATTERN.match(fileName))
    if len(characterFiles) > 1:
        raise ProcessException("Multiple character files found: %s" % ', '.join(characterFiles))
    if characterFiles:
        print "Character file found: %s, verifying" % characterFiles[0]
        # ToDo verifyCharacter(emotions, join(armletDir, characterFiles[0]))
    # Check music status
    (withErrors, markDate, okNum, okSize, errorText) = checkResultMark(baseDir)
    if markDate:
        try:
            # Verify that status mark is still actual
            if any(date > markDate for date in (getFileModificationTime(f) for f in chain((sourceDir, musicDir), sourceFiles, musicFiles, errorFiles))):
                raise ProcessException("Status mark obsolete, newer music files exist")
            if okNum is None:
                raise ProcessException("No music files found")
            if okNum != len(musicFiles):
                raise ProcessException("Existing record mentions %s files, but %d is actually found" % (okNum, len(musicFiles)))
            if okSize != sum(getsize(f) for f in musicFiles):
                raise ProcessException("Existing record mentions total file size %d bytes, while actual total size is %d bytes" % (okSize, sum(getsize(f) for f in musicFiles)))
            # Verify existing music files
            print "Verifying files",
            foundEmotions = set()
            for fileName in listdir(musicDir):
                stdout.write('.')
                stdout.flush()
                fullName = join(musicDir, fileName)
                dumpToErrors = False
                match = CHECK_PATTERN.match(fileName)
                if match:
                    groups = match.groupdict()
                    emotion = convertEmotion(groups[EMOTION])
                    artist = convertTitle(groups[ARTIST])
                    title = convertTitle(groups[TITLE] or '')
                    tail = convert(groups[TAIL] or '')
                    if emotion not in emotions:
                        raise ProcessException("\nUnknown emotion: %d" % emotion)
                    foundEmotions.add(emotion)
                else:
                    raise ProcessException("\nBad file name: %s" % fileName)
                if verifyFiles:
                    e = verifyFile(join(musicDir, fileName))
                    if e:
                        raise ProcessException("\nError processing: %s" % e)
            print
            for emotion in (e for e in emotions if e not in foundEmotions):
                print "WARNING: Emotion %s is missing" % emotion.upper()
        except ProcessException, e:
            print "%s, reprocessing" % e
            resultMark(baseDir, None)
            markDate = None
    if markDate:
        hasMusic = True
        print "Music already processed %s, %s files found (%d total size)" % ('OK' if not withErrors else 'with ERRORS', okNum, okSize)
        if errorText:
            print errorText.strip()
    elif not isdir(sourceDir):
        hasMusic = False
        log(True, None, "No music source directory found: %s" % sourceDir)
    else:
        # Process source music
        files = deepListDir(sourceDir)
        hasMusic = bool(files)
        foundEmotions = set()
        if not hasMusic:
            log(True, None, "No music files found in source directory: %s" % sourceDir)
        else:
            log(False, None, "Source music files found: %d" % len(files))
            createDir(musicDir)
            if isdir(errorDir):
                for f in chain(getFiles(musicDir), getFiles(errorDir)):
                    remove(f)
                rmdir(errorDir)
            for (dirName, fileName) in files:
                stdout.write('.')
                stdout.flush()
                fullName = join(dirName, fileName)
                dumpToErrors = False
                match = CHECK_PATTERN.match(fileName)
                if match:
                    groups = match.groupdict()
                    emotion = groups[EMOTION]
                    artist = convertTitle(groups[ARTIST])
                    title = convertTitle(groups[TITLE] or '')
                    tail = convert(groups[TAIL] or '')
                    if not title:
                        title = artist
                        artist = ''
                    if emotion not in emotions:
                        log(True, fileName, "Unknown emotion")
                        dumpToErrors = True
                    foundEmotions.add(emotion)
                    newFileNamePrefix = SEPARATOR.join((emotion, character.shortName))
                    for s in (artist, title, tail):
                        if s:
                            newFileNamePrefix += SEPARATOR + s
                    newFileNamePrefix = cleanupFileName(newFileNamePrefix)[:MAX_FILE_NAME]
                    for i in count():
                        newFileName = '%s%s%s' % (newFileNamePrefix, i or '', NEW_EXTENSION)
                        if newFileName.lower() not in newFileNameSet:
                            break
                    newFileNameSet.add(newFileName.lower())
                    newFullName = join(musicDir, newFileName)
                else:
                    log(True, fileName, "Bad file name")
                    dumpToErrors = True
                    newFileName = fileName
                if dumpToErrors:
                    createDir(errorDir)
                    newFullName = join(errorDir, newFileName)
                if match:
                    e = processFile(fullName, newFullName)
                else:
                    e = True
                if e:
                    if e != True:
                        log(True, fileName, "Error processing: %s" % e)
                    createDir(errorDir)
                    copy(fullName, errorDir)
            print
            obsoleteFiles = tuple(f for f in listdir(musicDir) if f.lower() not in newFileNameSet)
            if obsoleteFiles:
                print "Obsolete files found (%d), removing:" % len(obsoleteFiles)
                for fileName in obsoleteFiles:
                    print encodeForConsole(fileName)
                    remove(join(musicDir, fileName))
            processedFiles = getFiles(musicDir)
            numProcessed = len(processedFiles)
            if numProcessed != len(files):
                log(True, None, "Processed file number mismatch: %d, expected %d" % (numProcessed, len(files)))
            for emotion in (e for e in emotions if e not in foundEmotions):
                print "WARNING: Emotion %s is missing" % emotion.upper()
            processedSize = sum(getsize(f) for f in processedFiles)
            resultMark(baseDir, hasErrors[0], numProcessed if hasMusic else None, processedSize if hasMusic else None, ''.join(messages))
            if numProcessed:
                print "Files OK: %d (%d total size)" % (numProcessed, processedSize)
    return (hasMusic, hasErrors[0])

def updateMusic(sourceDir = '.', verifyFiles = False):
    sourceDir = expanduser(sourceDir)
    if not isdir(sourceDir):
        print "Music directory not found: %s" % sourceDir
        return
    if not isfile(join(sourceDir, MUSIC_MARK)):
        print "Not a music directory: %s" % sourceDir
        return
    characterDirs = [str(d) for d in listdir(unicode(sourceDir)) if d not in EXCLUDE_DIRS and isdir(join(sourceDir, d))]
    (emotions, characters) = updateEmotions()
    okCharacters = []
    unknownCharacters = tuple(sorted(d for d in characterDirs if d not in characters))
    for name in (name for name in characters if name not in characterDirs):
        characterDirs.append(name)
        createDir(join(sourceDir, name, SOURCE_DIR))
    characterDirs = tuple(sorted(characterDirs))
    print
    print "Processing music at %s" % sourceDir
    print "Known characters found: %d" % len(characters)
    print "Character directories found: %d%s" % (len(characterDirs), (' (%d unknown)' % len(unknownCharacters)) if unknownCharacters else '')
    noMusicCharacters = []
    errorCharacters = []
    for d in characterDirs:
        character = characters.get(d)
        if not character:
            character = CharacterCSVable({'shortName': d})
        (hasMusic, hasErrors) = processCharacter(character, emotions, sourceDir, verifyFiles)
        if hasMusic:
            if not hasErrors:
                okCharacters.append(d)
            if not character.hasMusic:
                print "WARNING: Character has music but is not marked so in JoinRPG"
        else:
            noMusicCharacters.append(d)
            if character.hasMusic:
                print "WARNING: Character does not have music but is marked so in JoinRPG"
        if hasErrors:
            errorCharacters.append(d)
    if okCharacters:
        print "\nOK character directories found (%d): %s" % (len(okCharacters), ', '.join(okCharacters))
    if unknownCharacters:
        print "\nUnknown character directories found (%d): %s" % (len(unknownCharacters), ', '.join(unknownCharacters))
    if noMusicCharacters:
        print "\nNo music found for characters (%d): %s" % (len(noMusicCharacters), ', '.join(sorted(noMusicCharacters)))
    if errorCharacters:
        print "\nErrors detected with music for characters (%d): %s" % (len(errorCharacters), ', '.join(sorted(errorCharacters)))
    print

def main(*args):
    verifyFiles = False
    (options, parameters) = getopt(args, 'v', ('verify',))
    for (option, _value) in options:
        if option in ('-v', '--verify'):
            verifyFiles = True
    updateMusic(parameters[0] if parameters else getenv(MUSIC_LOCATION_VARIABLE, '.'), verifyFiles)

if __name__ == '__main__':
    main(*argv[1:])

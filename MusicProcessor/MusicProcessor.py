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
from os.path import basename, expanduser, getmtime, getsize, isdir, isfile, islink, join, relpath
from re import compile as reCompile
from shutil import copy, rmtree
from sys import argv, platform, stdout
from time import mktime
from traceback import format_exc

try: # Filesystem symbolic links configuration
    from os import link as hardlink, symlink # UNIX # pylint: disable=E0611,C0412,W0611
except ImportError:
    try:
        from ctypes import windll # Windows
        dll = windll.LoadLibrary('kernel32.dll')
        def hardlink(source, linkName):
            if not dll.CreateHardLinkW(linkName, source, None):
                raise OSError("code %d" % dll.GetLastError())
        def symlink(source, linkName):
            if not dll.CreateSymbolicLinkW(linkName, source, isdir(source)):
                raise OSError("code %d" % dll.GetLastError())
    except Exception as ex:
        raise ImportError("%s: %s\n\nFilesystem links are not available.\nPlease run on UNIX or Windows Vista or later.\n" % (ex.__class__.__name__, ex))

try:
    from pydub import AudioSegment
except ImportError as ex:
    raise ImportError("%s: %s\n\nPlease install pydub v0.20 or later: https://pypi.python.org/pypi/pydub\n" % (ex.__class__.__name__, ex))

from CharacterProcessor import CharacterCSVable

from EmotionConverter import convertEmotion, convertTitle
from EmotionProcessor import updateEmotions

#
# ToDo: Make sure personal music doesn't contain (warn about them) common track names like smert', tuman etc.
#

MUSIC_LOCATION_VARIABLE = 'DTAH_MUSIC'

SOURCE_DIR = 'src'
ARMLET_DIR = 'armlet'
MUSIC_DIR = 'personal'
COMMON_MUSIC_DIR = 'common'
ERROR_DIR = 'errors'
OTHER_DIR = 'other'

COMMON_DIR = '_COMMON'

INI_FILE = 'settings.ini'

INI_CONTENT = open('settings_ini.tpl').read().replace('\r\n', '\n').replace('\n', '\r\n')

CHARACTER_CSV_PATTERN = reCompile(r'character.*\.csv')

SEPARATOR = '-'

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

def silentRemove(fileName):
    print "- Removing %s" % fileName
    try:
        remove(fileName)
    except OSError as e:
        if e.errno != ENOENT:
            raise

def createDir(dirName):
    if isdir(dirName):
        print "- Directory %s exists" % dirName
    else:
        print "- Creating directory %s" % dirName
        makedirs(dirName)

def copyTo(what, where):
    print "- Creating link to %s in %s" % (what, where)
    assert isdir(where)
    symlink(relpath(what, where), join(where, basename(what)))

def deepListDir(dirName):
    ret = tuple(chain.from_iterable(((dirPath, fileName) for fileName in fileNames) for (dirPath, dirNames, fileNames) in walk(dirName)))
    print "- Got deep list of directory %s: %d files found" % (dirName, len(ret))
    return ret

def getFiles(dirName):
    ret = tuple(join(dirName, f) for f in listdir(dirName)) if isdir(dirName) else ()
    print "- Got files from directory %s: %d files found" % (dirName, len(ret))
    return ret

def deepGetFiles(dirName):
    return tuple(join(d, f) for (d, f) in deepListDir(dirName)) if isdir(dirName) else ()

def parseFileName(fileName, isCharacter = False):
    ret = [] if isCharacter else [None,]
    for (n, token) in enumerate(fileName.rsplit('.', 1)[0].split(SEPARATOR)):
        token = token.strip()
        if n == 0:
            token = token.lower() if isCharacter else token.upper()
        elif n == 1 and isCharacter:
            token = token.upper()
        ret.append(token)
    return (ret[0], ret[1] if ret[1:] else None, ret[2:])

def processFile(fullName, newFullName):
    try:
        sourceAudio = AudioSegment.from_file(fullName)
        if sourceAudio.duration_seconds < 1:
            return "Audio too short: %d seconds" % sourceAudio.duration_seconds
        if sourceAudio.duration_seconds < 5:
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
    except Exception as e:
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
    except Exception as e:
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
            okText = tuple(int(i) for i in f.read().split(',')) # pylint: disable=R0204
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
    if character is None:
        print "\nProcessing common directory"
    else:
        print "\nProcessing character: %s (%s)" % (character.shortName, character.rID if character.rID is not None else 'UNKNOWN')
    messages = []
    hasErrors = [False]
    commonDir = join(unicode(baseDir), COMMON_DIR)
    commonMusicDir = join(commonDir, COMMON_MUSIC_DIR)
    if character:
        baseDir = join(unicode(baseDir), character.shortName)
    else:
        baseDir = commonDir
    sourceDir = join(baseDir, SOURCE_DIR)
    errorDir = join(baseDir, ERROR_DIR)
    if character:
        otherDir = join(commonDir, OTHER_DIR)
        armletDir = join(baseDir, ARMLET_DIR)
        musicDir = join(armletDir, MUSIC_DIR)
        createDir(armletDir)
    else:
        armletDir = None
        musicDir = commonMusicDir
    sourceFiles = deepGetFiles(sourceDir)
    musicFiles = getFiles(musicDir)
    errorFiles = getFiles(errorDir)
    newFileNameSet = set()
    if character:
        # Removing common files
        for fileName in (join(armletDir, f) for f in listdir(armletDir) if f != MUSIC_DIR and not CHARACTER_CSV_PATTERN.match(f)):
            # ToDo: Exclude other CSV files
            if isdir(fileName) and not islink(fileName):
                print "- Removing directory %s" % fileName
                rmtree(fileName)
            else:
                print "- Removing file %s" % fileName
                remove(fileName)
        # Copying common files
        for fileName in listdir(otherDir):
            copyTo(join(commonDir, fileName), armletDir)
        # Creating settings.ini
        if character.rID > 0:
            with open(join(armletDir, INI_FILE), 'wb') as f:
                fields = character.getFields()
                fields['kaTetIDs'] = character.kaTetIDs
                f.write(INI_CONTENT % fields)
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
                (ch, emotion, tail) = parseFileName(fileName, character)
                if emotion and (not character or ch == character):
                    if emotion not in emotions:
                        raise ProcessException("\nUnknown emotion: %s in file name %s" % (emotion, fileName))
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
        except ProcessException as e:
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
                (ch, emotion, tail) = parseFileName(fileName, character)
                if emotion and (not character or ch.lower() == character.lower()):
                    emotion = convertEmotion(emotion)
                    if emotion not in emotions:
                        log(True, fileName, "Unknown emotion")
                        dumpToErrors = True
                    foundEmotions.add(emotion)
                    newFileNamePrefix = cleanupFileName(SEPARATOR.join(chain((character.shortName, emotion) if character else (emotion,), (convertTitle(t) for t in tail))))[:MAX_FILE_NAME]
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
                if newFullName:
                    e = processFile(fullName, newFullName)
                else:
                    e = True # pylint: disable=R0204
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
    characterDirs = [str(d) for d in listdir(unicode(sourceDir)) if d != COMMON_DIR and isdir(join(sourceDir, d))]
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
    processCharacter(None, emotions, sourceDir, verifyFiles)
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

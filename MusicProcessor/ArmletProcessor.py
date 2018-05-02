#!/usr/bin/python
#
# Armlet Processor for "Dark Tower: All Hail" LARP.
#
# - Uses CharacterProcessor.py to update character information.
# - Uses EmotionProcessor.py to update emotions information.
# - Updates and verifies information in DTAH Music Directory
#   specified with DTAH_MUSIC environment variable, with a parameter
#   or in current directory.
# - DTAH Music directory must contain DTAH_Music_Here file.
#
# Usage:
# - python ArmletProcessor.py [musicDirectory]
#
from os import getenv, listdir, makedirs, remove
from os.path import expanduser, isdir, isfile, join
from shutil import copy
from sys import argv, path as sysPath
from traceback import format_exc

from CharacterProcessor import CharacterCSVable

from EmotionProcessor import updateEmotions

MUSIC_LOCATION_VARIABLE = u'DTAH_MUSIC'

MUSIC_MARK = 'DTAH_Music_Here'

COMMON_DIR = u'_COMMON'

INI_FILE = u'settings.ini'

INI_CONTENT = open('settings_ini.tpl').read().replace('\r\n', '\n').replace('\n', '\r\n')

GENERATED_FILES = ('Emotions.csv', 'Reasons.csv', 'Characters.csv')

def processCharacter(character, baseDir = '.'):
    print "Processing character: %s (%s)" % (character.shortName, character.rID if character.rID is not None else 'UNKNOWN')
    commonDir = join(unicode(baseDir), COMMON_DIR)
    charDir = join(unicode(baseDir), character.shortName)

    # Clearing directory contents
    for fileName in listdir(charDir):
        if isdir(join(charDir, fileName)):
            print "Found directory", fileName
        else:
            #print "Removing", fileName
            remove(join(charDir, fileName))

    # Creating settings.ini
    if character.rID:
        #print "Generating", INI_FILE
        with open(join(charDir, INI_FILE), 'wb') as f:
            fields = character.getFields()
            fields['kaTetIDs'] = character.kaTetIDs
            f.write(INI_CONTENT % fields)

    # Copying common files
    for fileName in GENERATED_FILES:
        #print "Copying", fileName
        copy(join(sysPath[0], fileName), charDir)

    # Copying generated files
    for fileName in listdir(commonDir):
        #print "Copying", fileName
        copy(join(commonDir, fileName), charDir)

def updateMusic(sourceDir = '.'):
    (_emotions, characters) = updateEmotions()
    print
    print "Running DTAH ArmletProcessor"
    print
    sourceDir = expanduser(unicode(sourceDir))
    if not isdir(sourceDir):
        print "DTAH Music directory not found: %s" % sourceDir
        return
    if not isfile(join(sourceDir, MUSIC_MARK)):
        print "Not a DTAH music directory: %s" % sourceDir
        return
    print "Processing armlets at %s" % sourceDir
    print "Known characters found: %d" % len(characters)
    characterDirs = [d for d in listdir(sourceDir) if d != COMMON_DIR and isdir(join(sourceDir, d))]
    okCharacters = []
    errorCharacters = []
    unknownCharacters = tuple(sorted(d for d in characterDirs if d not in characters))
    for name in (name for name in characters if name not in characterDirs):
        dirName = join(sourceDir, name)
        print "Creating directory %s" % dirName
        makedirs(dirName)
        characterDirs.append(name)
    characterDirs = tuple(sorted(characterDirs))
    print "Character directories found: %d%s" % (len(characterDirs), (' (%d unknown)' % len(unknownCharacters)) if unknownCharacters else '')
    print
    for d in characterDirs:
        character = characters.get(d)
        if not character:
            character = CharacterCSVable({'shortName': d})
        try:
            processCharacter(character, sourceDir)
            okCharacters.append(d)
        except Exception:
            errorCharacters.append(d)
            print format_exc()
    if okCharacters:
        print "\nOK character directories found (%d): %s" % (len(okCharacters), ', '.join(okCharacters))
    if unknownCharacters:
        print "\nUnknown character directories found (%d): %s" % (len(unknownCharacters), ', '.join(unknownCharacters))
    if errorCharacters:
        print "\nErrors detected with music for characters (%d): %s" % (len(errorCharacters), ', '.join(sorted(errorCharacters)))

def main(*args):
    updateMusic(args[0] if args else getenv(MUSIC_LOCATION_VARIABLE, '.'))

if __name__ == '__main__':
    main(*argv[1:])

#!/usr/bin/python
#
# JoinRPG.ru X-API
#
from collections import OrderedDict
from datetime import datetime, timedelta
from getpass import getpass
from json import loads as jsonLoads, JSONEncoder
from os import getcwd
from os.path import dirname, isfile, join, realpath
from sys import argv, stdout

# Requests HTTP library
try:
    import requests
except ImportError as ex:
    print "%s: %s\nERROR: This software requires Requests library.\nPlease install Requests: https://pypi.python.org/pypi/requests" % (ex.__class__.__name__, ex)
    exit(-1)

from JSONable import JSONable

API_DATE_FORMAT = '%Y-%m-%dT%H:%M:%S.%f'

CACHE_FILE_NAME = 'joinrpg-%d.json'
DEBUG_DUMP_FILE_NAME = 'joinrpg-%d-debug.json'

def getWorkingDir():
    if argv[0].endswith('.py'):
        return dirname(realpath(argv[0]))
    return getcwd()

def getFileName(name):
    return join(getWorkingDir(), name)

def date2str(date):
    return date.strftime(API_DATE_FORMAT)[:-3]

def str2date(s):
    if s is None or isinstance(s, datetime):
        return s
    return datetime.strptime(s, API_DATE_FORMAT)

class MoreJSONEncoder(JSONEncoder):
    def default(self, o): # pylint: disable=E0202
        if isinstance(o, set):
            return tuple(o)
        if isinstance(o, datetime):
            return date2str(o)
        if isinstance(o, JSONable):
            return o.getFields()
        return JSONEncoder.default(self, o)

class Character(JSONable):
    IGNORE_FIELDS = ('joinRPG',)

    def __init__(self, joinRPG, **kwargs):
        self.joinRPG = joinRPG
        self.fieldValues = {}
        JSONable.__init__(self, **kwargs)

    def _processFields(self):
        if not self.characterLink.startswith(self.joinRPG.joinRPG): # pylint: disable=E0203
            self.characterLink = '%s%s' % (self.joinRPG.joinRPG, self.characterLink)
        self.updatedAt = str2date(self.updatedAt)
        self.groupNames = set(g.characterGroupName for g in self.groups) # pylint: disable=E1101
        for field in self.fields: # pylint: disable=E1101
            fieldName = self.joinRPG.fieldNames.get(field.projectFieldId)
            if fieldName is not None:
                self.fieldValues[fieldName] = field.displayString

class JoinRPG(JSONable):
    def __init__(self, projectId, username = None, password = None, joinRPG = 'http://joinrpg.ru', cacheData = False, cacheAuth = False, **kwargs):
        projectId = int(projectId)
        self.username = username
        self.password = password
        self.joinRPG = joinRPG
        self.cacheData = cacheData
        self.cacheAuth = cacheAuth
        self.cacheFileName = getFileName(CACHE_FILE_NAME % projectId)
        self.IGNORE_FIELDS = set(self.getFields())
        self.metadata = None
        self.fieldNames = None
        self.characterData = None
        self.characters = None
        self.updatedAt = None
        self._resetAuth()
        self.OUTPUT_FIELDS = (('accessToken', 'accessTokenType', 'accessTokenExpiresAt') if cacheAuth else ()) \
                           + (('updatedAt', 'metadata', 'characterData') if cacheData else ())
        self.projectId = projectId
        JSONable.__init__(self, **kwargs)
        self.loadCache()

    def _processFields(self):
        self.updatedAt = str2date(self.updatedAt)
        self.accessTokenExpiresAt = str2date(self.accessTokenExpiresAt)

    def _resetAuth(self):
        self.accessToken = self.accessTokenType = self.accessTokenExpiresAt = None

    def loadCache(self):
        if self.cacheData or self.cacheAuth:
            if isfile(self.cacheFileName):
                print "Loading cache for project ID %d..." % self.projectId
                with open(self.cacheFileName) as f:
                    json = f.read()
                if json:
                    for (field, value) in jsonLoads(json).iteritems():
                        setattr(self, field, value)
                    self._processFields()
                else:
                    print "The cache is empty"
            else:
                print "No cache found for project ID %d" % self.projectId
        if not self.cacheAuth:
            self._resetAuth()
        if not self.cacheData:
            self.updatedAt = self.metadata = self.fieldNames = self.characterData = self.characters = None

    def saveCache(self):
        if self.cacheData or self.cacheAuth:
            print "Saving cache..."
            self.metadata = OrderedDict(sorted(self.metadata.iteritems(), key = lambda (field, value): {'ProjectId': 0, 'ProjectName': 1}.get(field, field)))
            self.characterData = sorted(self.characterData, key = lambda character: character['CharacterId'])
            with open(self.cacheFileName, 'w') as f:
                f.write(self.json(isOutput = True, cls = MoreJSONEncoder, indent = 4))

    def saveDebugDump(self):
        print "Saving debug dump..."
        with open(getFileName(DEBUG_DUMP_FILE_NAME % self.projectId), 'w') as f:
            f.write(self.json(sort_keys = True, cls = MoreJSONEncoder, indent = 4))

    def authorize(self):
        if None in (self.username, self.password):
            print "Authentication needed, please enter your credentials"
            if self.username is None:
                self.username = raw_input("Username: ")
            if self.password is None:
                self.password = getpass("Password: ")
        response = requests.post('%s/x-api/token' % self.joinRPG,
                {'grant_type': 'password', 'username': self.username, 'password': self.password},
                {'Content-Type': 'application/x-www-form-urlencoded'})
        response.raise_for_status()
        json = response.json()
        self.accessToken = str(json['access_token'])
        if not self.accessToken:
            raise ValueError("Access token is empty")
        self.accessTokenType = str(json['token_type'])
        expiresSeconds = int(json['expires_in'])
        if expiresSeconds < 1:
            raise ValueError("Access token expires too fast: in %d seconds" % expiresSeconds)
        self.accessTokenExpiresAt = datetime.utcnow() + timedelta(seconds = expiresSeconds - 1)

    def getData(self, url):
        wasUnauthorized = not self.accessToken or datetime.utcnow() >= self.accessTokenExpiresAt
        if wasUnauthorized:
            self.authorize()
        while True:
            response = requests.get(url, headers = {'Authorization': '%s %s' % (self.accessTokenType, self.accessToken)})
            if wasUnauthorized or response.status_code != requests.codes.unauthorized: # pylint: disable=E1101
                break
            self.authorize()
        response.raise_for_status()
        return response.json()

    def getMetadata(self):
        print "Getting metadata..."
        metadata = None
        try:
            metadata = self.getData('%s/x-game-api/%d/metadata/fields' % (self.joinRPG, self.projectId))
        except Exception, e:
            print "ERROR: %s" % e
            if self.metadata:
                print "Using cached metadata"
            else:
                print "Aborting"
                raise
        print "Project name is: %s" % (metadata or self.metadata)['ProjectName']
        if metadata:
            if metadata == self.metadata:
                print "Metadata unchanged, updating cache"
            else:
                if self.metadata is not None:
                    print "Metadata has changed, discarding cache"
                self.metadata = metadata
                self.updatedAt = self.characterData = self.characters = None
        self._parseFields(**self.metadata)
        self.fieldNames = OrderedDict((field.projectFieldId, field.fieldName) for field in self.fields if field.isActive) # pylint: disable=E1101

    def getCharacters(self, modifiedSince = None):
        self.getMetadata()
        if modifiedSince is None or self.updatedAt is not None and modifiedSince <= self.updatedAt:
            modifiedSince = self.updatedAt if self.characterData else None
            self.updatedAt = datetime.utcnow()
        modifiedSinceStr = date2str(modifiedSince) if modifiedSince else None
        print "Getting character data%s..." % ((' modified since %s' % modifiedSinceStr) if modifiedSinceStr else '')
        if self.characterData is None:
            self.characterData = []
        newCharacters = None
        try:
            newCharacters = self.getData('%s/x-game-api/%d/characters/%s' % (self.joinRPG, self.projectId,
                                        ('?modifiedSince=%s' % modifiedSinceStr) if modifiedSinceStr else ''))
        except Exception, e:
            print "ERROR: %s" % e
            if self.metadata:
                print "Using cached character data"
            else:
                print "Aborting"
                raise
        if newCharacters:
            print "Getting details for %d characters... " % len(newCharacters),
            for newCharacter in newCharacters:
                newCharacter.update(self.getData('%s%s' % (self.joinRPG, newCharacter['CharacterLink'])))
                characterID = newCharacter['CharacterId']
                for (index, oldCharacter) in enumerate(self.characterData):
                    if oldCharacter['CharacterId'] == characterID:
                        self.characterData[index] = newCharacter
                        break
                else:
                    self.characterData.append(newCharacter)
                stdout.write('.')
                stdout.flush()
            print
        elif newCharacters is not None:
            print "No updates found"
        self.characters = tuple(sorted(Character.fromIterable(self.characterData, self), key = lambda character: character.characterId))
        self.saveCache()
        self.saveDebugDump()
        return self.characters

def getAllCharacters(*args, **kwargs):
    """Returns all characters for a project with the specified ID as iterator of Character objects."""
    if not args:
        raise ValueError("Project ID is not specified")
    return JoinRPG(*args, **kwargs).getCharacters()

def main():
    getAllCharacters(*argv[1:], cacheData = True, cacheAuth = True)

if __name__ == '__main__':
    main()

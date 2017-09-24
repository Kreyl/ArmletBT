#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# CSVable.py
#
# A collection of classes to read and write objects from and to CSV files.
#
from collections import OrderedDict
from csv import DictReader, DictWriter
from os.path import isfile

class CSVable(object):
    """Interface of an object that can be read from or written to a CSV file row."""

    CSV_FIELDS = None # or iterable of ASCII field names or {csvField: asciiObjectField,} dictionary

    READ_REST_KEY = 'REST'
    READ_REST_VAL = None
    WRITE_REST_VAL = ''
    WRITE_EXTRAS_ACTION = 'raise'

    def __init__(self, fields = None):
        for field in self._getFields():
            setattr(self, field, None)
        for (field, value) in (fields or {}).iteritems():
            if field and field[0].islower() or field == self.READ_REST_KEY:
                setattr(self, field, value)

    def _getFields(self): # generator
        if self.CSV_FIELDS is None:
            return (field for field in self.__dict__ if field[0].islower() and not hasattr(field, '__call__') or field == self.READ_REST_KEY)
        if isinstance(self.CSV_FIELDS, dict):
            return self.CSV_FIELDS.itervalues()
        return self.CSV_FIELDS # CSV_FIELDS is plane iterable

    def _getFieldsValues(self): # generator
        return ((field, getattr(self, field)) for field in self._getFields())

    def __str__(self):
        return '{%s}' % ', '.join('%s: %s' % fv for fv in self._getFieldsValues())

    def __repr__(self):
        return '%s({%s})' % (self.__class__.__name__, ', '.join('%r: %r' % fv for fv in self._getFieldsValues()))

    def __eq__(self, other):
        return self.__class__ is other.__class__ and sorted(self._getFieldsValues()) == sorted(other._getFieldsValues()) # pylint: disable=W0212

class CSVfileable(CSVable):
    """Interface of a class whose instances can be bunch-read from or written to a CSV file."""

    INSTANCES = None
    FILE_NAME = None
    NEEDS_HEADER = None
    ENCODING = None
    HEADER_COMMENT = None
    KEY_FUNCTION = None

    @classmethod
    def getInstances(cls):
        return cls.INSTANCES

    @classmethod
    def getFileName(cls):
        return cls.FILE_NAME

    @classmethod
    def getNeedsHeader(cls):
        return cls.NEEDS_HEADER

    @classmethod
    def getEncoding(cls):
        return cls.ENCODING

    @classmethod
    def getHeaderComment(cls):
        return cls.HEADER_COMMENT

    @classmethod
    def _sort(cls, keyFunction):
        if isinstance(cls.INSTANCES , dict):
            cls.INSTANCES = OrderedDict((key, value) for (key, value) in sorted(cls.INSTANCES.iteritems(), key = lambda (_key, value): keyFunction(value)))
        else:
            cls.INSTANCES.sort(key = keyFunction) # pylint: disable=E1101
        return cls.INSTANCES

    @classmethod
    def sort(cls):
        return cls._sort(cls.sortKey)

    def sortKey(self):
        return tuple(self._getFieldsValues())

    @classmethod
    def loadCSV(cls, fileName = None, useHeader = None, encoding = None, handleComments = None, keyFunction = None, *args, **kwargs):
        """Loads instances of this class from a CSV file."""
        if fileName is None:
            fileName = cls.getFileName()
        if useHeader is None:
            useHeader = cls.getNeedsHeader()
        if encoding is None:
            encoding = cls.getEncoding()
        if handleComments is None:
            handleComments = cls.getHeaderComment() is not None
        if keyFunction is None:
            keyFunction = cls.KEY_FUNCTION
        cls.INSTANCES = OrderedDict() if keyFunction else []
        if not isfile(fileName):
            print "No file found"
            return ()
        with open(fileName, 'rb') as f:
            for obj in CSVObjectReader(f, cls, useHeader, encoding, handleComments, *args, **kwargs):
                if isinstance(cls.INSTANCES, list):
                    cls.INSTANCES.append(obj)
                else: # dict
                    cls.INSTANCES[keyFunction(obj)] = obj

    @classmethod
    def dumpCSV(cls, instances = None, fileName = None, writeHeader = None, encoding = None, headerComment = None, *args, **kwargs):
        """Dumps instances of this class to a CSV file."""
        if instances is None:
            instances = cls.getInstances()
        if fileName is None:
            fileName = cls.getFileName()
        if writeHeader is None:
            writeHeader = cls.getNeedsHeader()
        if encoding is None:
            encoding = cls.getEncoding()
        if headerComment is None:
            headerComment = cls.getHeaderComment()
        with open(fileName, 'wb') as f:
            CSVObjectWriter(f, cls, writeHeader, encoding, headerComment, *args, **kwargs).writerows(instances)

class CSVableParser(object):
    """Intermediate class that is used to recognize and setup structure of a CSV file."""

    COMMENT = '#'

    def __init__(self, csvAbleClass = CSVable, encoding = None):
        self.encoding = encoding or 'ascii'
        self.csvAbleClass = csvAbleClass
        if csvAbleClass.CSV_FIELDS is None: # Use class fields to determine CSV content
            self.initFromObject(csvAbleClass)
        else: # Use CSV_FIELDS to determine CSV content
            if isinstance(csvAbleClass.CSV_FIELDS, dict):
                self.validateObjectFields(csvAbleClass.CSV_FIELDS.itervalues(), True)
                iterFields = csvAbleClass.CSV_FIELDS.iteritems()
                if not isinstance(csvAbleClass.CSV_FIELDS, OrderedDict):
                    iterFields = sorted(iterFields)
            else: # csvAbleClass.CSV_FIELDS is plane iterable
                self.validateObjectFields(csvAbleClass.CSV_FIELDS, True)
                iterFields = zip(csvAbleClass.CSV_FIELDS, csvAbleClass.CSV_FIELDS)
            self.fieldsDict = {}
            self._fieldNames = []
            for (csvField, objectField) in iterFields:
                csvField = csvField.encode(self.encoding)
                self.fieldsDict[csvField] = objectField
                self._fieldNames.append(csvField)
            self._fieldNames = tuple(self._fieldNames) # pylint: disable=E0012,R0204

    def initFromObject(self, obj):
        fields = tuple(sorted(field for field in obj.__dict__ if field[0].islower() and not hasattr(field, '__call__')))
        self.fieldsDict = dict(zip(fields, fields)) or None
        self._fieldNames = fields or None

    @staticmethod
    def validateObjectFields(objectFields, isStrict = False):
        fieldSet = set()
        for objectField in objectFields:
            if not objectField:
                if isStrict:
                    raise ValueError("Object field name is empty string")
                continue
            try:
                objectField = objectField.encode('ascii')
            except UnicodeError:
                raise ValueError("Object field name is not ASCII: %r" % objectField)
            if objectField[0].isupper():
                objectField = objectField[0].lower() + objectField[1:]
            elif not objectField[0].islower():
                raise ValueError("Object field name does not start with a letter: %s" % objectField)
            if objectField in fieldSet:
                raise ValueError("Duplicate object field name: %s" % objectField)
            fieldSet.add(objectField)

class CSVObjectReader(CSVableParser, DictReader):
    """CSV reader that reads rows from a CSV file and returns them as CSVable objects."""

    def __init__(self, csvFile, csvAbleClass = CSVable, useHeader = False, encoding = None, handleComments = False, *args, **kwargs):
        CSVableParser.__init__(self, csvAbleClass, encoding)
        if handleComments:
            csvFile = (line for line in csvFile if not line.strip().startswith(self.COMMENT))
        DictReader.__init__(self, csvFile, None if useHeader else self._fieldNames or (), csvAbleClass.READ_REST_KEY, csvAbleClass.READ_REST_VAL, *args, **kwargs)
        if useHeader and self.fieldnames and self._fieldNames is None:
            self.validateObjectFields(self.fieldnames)

    def next(self):
        ret = self.csvAbleClass()
        for (csvField, value) in DictReader.next(self).iteritems():
            if isinstance(value, str):
                value = value.decode(self.encoding)
                try:
                    value = value.encode('ascii')
                except UnicodeError:
                    pass
            field = self.fieldsDict.get(csvField) if self.fieldsDict and csvField != self.csvAbleClass.READ_REST_KEY else csvField
            if field:
                setattr(ret, field, value)
            elif value is not None:
                rest = getattr(ret, self.csvAbleClass.READ_REST_KEY, None)
                if rest is None:
                    rest = value if field == self.csvAbleClass.READ_REST_KEY else {csvField: value}
                    setattr(ret, self.csvAbleClass.READ_REST_KEY, rest)
                elif field == self.csvAbleClass.READ_REST_KEY:
                    rest.update(value)
                else:
                    rest[csvField] = value
        return ret

class CSVObjectWriter(CSVableParser, DictWriter):
    """CSV writer that can write CSVable objects to a CSV file."""

    def __init__(self, csvFile, csvAbleClass = CSVable, writeHeader = False, encoding = None, headerComment = None, *args, **kwargs):
        CSVableParser.__init__(self, csvAbleClass, encoding)
        if headerComment:
            lines = (line.strip().encode(self.encoding) for line in (headerComment.splitlines() if isinstance(headerComment, str) else headerComment or ())) # pylint: disable=C0325
            csvFile.write(''.join('%s\r\n' % (line if line.startswith(self.COMMENT) else '%s %s' % (self.COMMENT, line) if line else self.COMMENT) for line in lines))
        DictWriter.__init__(self, csvFile, self._fieldNames, csvAbleClass.WRITE_REST_VAL, csvAbleClass.WRITE_EXTRAS_ACTION, *args, **kwargs)
        self.hasFieldNames = self.fieldnames is not None
        self.needHeader = writeHeader

    def writerow(self, obj): # pylint: disable=W0221
        def value(v):
            if v is None:
                return ''
            if not isinstance(v, str) and not isinstance(v, unicode):
                v = str(v)
            return v.encode(self.encoding)
        if not self.hasFieldNames:
            self.initFromObject(obj)
            self.fieldsDict = self.fieldsDict or {}
            self.fieldnames = self._fieldNames or ()
            self.hasFieldNames = True
        if self.needHeader:
            self.needHeader = False
            DictWriter.writerow(self, dict(zip(self.fieldnames, self.fieldnames)))
        DictWriter.writerow(self, dict((csvField, value(getattr(obj, objectField))) for (csvField, objectField) in self.fieldsDict.iteritems()))

    def writerows(self, rowdicts):
        for rowdict in rowdicts:
            self.writerow(rowdict)

    def writeheader(self):
        pass # This method is not needed in this implementation as the header gets written automatically

#
# TESTS
#

def testCSVableParser():
    class TestCSVable(CSVable):
        pass
    p = CSVableParser()
    assert p.fieldsDict is None, p.fieldsDict
    assert p._fieldNames is None, p._fieldNames # pylint: disable=W0212
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict is None, p.fieldsDict
    assert p._fieldNames is None, p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = {}
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict == {}, p.fieldsDict
    assert p._fieldNames == (), p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = () # pylint: disable=E0012,R0204
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict == {}, p.fieldsDict
    assert p._fieldNames == (), p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = {'aaa': 'bbb', 'ccc': 'ddd'}
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict == {'aaa': 'bbb', 'ccc': 'ddd'}, p.fieldsDict
    assert p._fieldNames == ('aaa', 'ccc'), p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = OrderedDict((('aaa', 'bbb'), ('ccc', 'ddd')))
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict == {'aaa': 'bbb', 'ccc': 'ddd'}, p.fieldsDict
    assert p._fieldNames == ('aaa', 'ccc'), p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = OrderedDict((('ccc', 'ddd'), ('aaa', 'bbb')))
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict == {'aaa': 'bbb', 'ccc': 'ddd'}, p.fieldsDict
    assert p._fieldNames == ('ccc', 'aaa'), p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = ['aaa', 'ccc']
    p = CSVableParser(TestCSVable)
    assert p.fieldsDict == {'aaa': 'aaa', 'ccc': 'ccc'}, p.fieldsDict
    assert p._fieldNames == ('aaa', 'ccc'), p._fieldNames # pylint: disable=W0212
    TestCSVable.CSV_FIELDS = {u'яяя': 'bbb', 'ccc': 'ddd'}
    p = CSVableParser(TestCSVable, encoding = 'utf-8')
    assert p.fieldsDict == {u'яяя'.encode('utf-8'): 'bbb', 'ccc': 'ddd'}, p.fieldsDict
    assert p._fieldNames == ('ccc', u'яяя'.encode('utf-8')), p._fieldNames # pylint: disable=W0212
    try:
        TestCSVable.CSV_FIELDS = {'aaa': '', 'ccc': 'ddd'}
        p = CSVableParser(TestCSVable)
        assert False, "No error"
    except ValueError:
        pass
    try:
        TestCSVable.CSV_FIELDS = {'aaa': u'ббб', 'ccc': 'ddd'}
        p = CSVableParser(TestCSVable)
        assert False, "No error"
    except ValueError:
        pass
    try:
        TestCSVable.CSV_FIELDS = ('', 'ccc')
        p = CSVableParser(TestCSVable)
        assert False, "No error"
    except ValueError:
        pass
    try:
        TestCSVable.CSV_FIELDS = (u'яяя', 'ccc')
        p = CSVableParser(TestCSVable)
        assert False, "No error"
    except ValueError:
        pass
    try:
        TestCSVable.CSV_FIELDS = {'aaa': 'bbb', 'ccc': 'bbb'}
        p = CSVableParser(TestCSVable)
        assert False, "No error"
    except ValueError:
        pass
    try:
        TestCSVable.CSV_FIELDS = ('aaa', 'aaa')
        p = CSVableParser(TestCSVable)
        assert False, "No error"
    except ValueError:
        pass

def testCSVObjectReader():
    class TestCSVable(CSVable):
        pass
    def checkCSVObjectReader(data, cls = CSVable, useHeader = False, *args, **kwargs):
        return tuple(CSVObjectReader(data.splitlines(), cls, useHeader, *args, **kwargs))
    result = checkCSVObjectReader('')
    assert result == (), result
    result = checkCSVObjectReader('', useHeader = True)
    assert result == (), result
    result = checkCSVObjectReader('\n\n\n')
    assert result == (), result
    result = checkCSVObjectReader('\n\n\n', useHeader = True)
    assert result == (), result
    result = checkCSVObjectReader('aaa,ccc,')
    assert result == (CSVable({'REST': ['aaa', 'ccc', '']}),), result
    result = checkCSVObjectReader('aaa,ccc,', useHeader = True)
    assert result == (), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n')
    assert result == (CSVable({'REST': ['aaa', 'ccc', '']}), CSVable({'REST': ['bbb', 'ddd']})), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', useHeader = True)
    assert result == (CSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), result
    result = checkCSVObjectReader('', TestCSVable)
    assert result == (), result
    result = checkCSVObjectReader('', TestCSVable, True)
    assert result == (), result
    TestCSVable.CSV_FIELDS = {}
    result = checkCSVObjectReader('', TestCSVable)
    assert result == (), result
    result = checkCSVObjectReader('', TestCSVable, True)
    assert result == (), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable)
    assert result == (TestCSVable({'REST': ['aaa', 'ccc', '']}), TestCSVable({'REST': ['bbb', 'ddd']})), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable, True)
    assert result == (TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), result
    TestCSVable.CSV_FIELDS = () # pylint: disable=E0012,R0204
    result = checkCSVObjectReader('', TestCSVable)
    assert result == (), result
    result = checkCSVObjectReader('', TestCSVable, True)
    assert result == (), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable)
    assert result == (TestCSVable({'REST': ['aaa', 'ccc', '']}), TestCSVable({'REST': ['bbb', 'ddd']})), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable, True)
    assert result == (TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), result
    TestCSVable.CSV_FIELDS = {'aaa': 'bbb', 'eee': 'fff'}
    result = checkCSVObjectReader('', TestCSVable)
    assert result == (), result
    result = checkCSVObjectReader('', TestCSVable, True)
    assert result == (), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable)
    assert result == (TestCSVable({'REST': [''], 'bbb': 'aaa', 'fff': 'ccc'}), TestCSVable({'bbb': 'bbb', 'fff': 'ddd'})), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable, True)
    assert result == (TestCSVable({'REST': ['ddd'], 'bbb': 'bbb'}),), result
    TestCSVable.CSV_FIELDS = OrderedDict((('eee', 'fff'), ('aaa', 'bbb')))
    result = checkCSVObjectReader('', TestCSVable)
    assert result == (), result
    result = checkCSVObjectReader('', TestCSVable, True)
    assert result == (), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable)
    assert result == (TestCSVable({'REST': [''], 'bbb': 'ccc', 'fff': 'aaa'}), TestCSVable({'bbb': 'ddd', 'fff': 'bbb'})), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable, True)
    assert result == (TestCSVable({'REST': ['ddd'], 'bbb': 'bbb'}),), result
    TestCSVable.CSV_FIELDS = ['ccc', 'aaa']
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable)
    assert result == (TestCSVable({'REST': [''], 'aaa': 'ccc', 'ccc': 'aaa'}), TestCSVable({'aaa': 'ddd', 'ccc': 'bbb'})), result
    result = checkCSVObjectReader('aaa,ccc,\nbbb,ddd\n', TestCSVable, True)
    assert result == (TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), result

def testCSVObjectWriter():
    from StringIO import StringIO
    class TestCSVable(CSVable):
        pass
    def checkCSVObjectWriter(objects, cls = CSVable, writeHeader = False, *args, **kwargs):
        ret = StringIO()
        CSVObjectWriter(ret, cls, writeHeader, *args, **kwargs).writerows(objects)
        return ret.getvalue()
    result = checkCSVObjectWriter(())
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), writeHeader = True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((CSVable(),))
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable(),), writeHeader = True)
    assert result == '\r\n\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable({'REST': ['aaa', 'ccc', '']}),))
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable({'REST': ['aaa', 'ccc', '']}),), writeHeader = True)
    assert result == '\r\n\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable({'aaa': 'bbb', 'ccc': 'ddd'}),))
    assert result == 'bbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), writeHeader = True)
    assert result == 'aaa,ccc\r\nbbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable(OrderedDict((('ccc', 'ddd'), ('aaa', 'bbb')))),))
    assert result == 'bbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((CSVable(OrderedDict((('ccc', 'ddd'), ('aaa', 'bbb')))),), writeHeader = True)
    assert result == 'aaa,ccc\r\nbbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((), TestCSVable)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), TestCSVable, True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable)
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable, True)
    assert result == '\r\n\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'REST': ['aaa', 'ccc', '']}),), TestCSVable)
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'REST': ['aaa', 'ccc', '']}),), TestCSVable, True)
    assert result == '\r\n\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), TestCSVable)
    assert result == 'bbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), TestCSVable, True)
    assert result == 'aaa,ccc\r\nbbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable(OrderedDict((('ccc', 'ddd'), ('aaa', 'bbb')))),), TestCSVable)
    assert result == 'bbb,ddd\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable(OrderedDict((('ccc', 'ddd'), ('aaa', 'bbb')))),), TestCSVable, True)
    assert result == 'aaa,ccc\r\nbbb,ddd\r\n', repr(result)
    TestCSVable.CSV_FIELDS = {}
    result = checkCSVObjectWriter((), TestCSVable)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), TestCSVable, True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable)
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable, True)
    assert result == '\r\n\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), TestCSVable)
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), TestCSVable, True)
    assert result == '\r\n\r\n', repr(result)
    TestCSVable.CSV_FIELDS = () # pylint: disable=E0012,R0204
    result = checkCSVObjectWriter((), TestCSVable)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), TestCSVable, True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable)
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable, True)
    assert result == '\r\n\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), TestCSVable)
    assert result == '\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'bbb', 'ccc': 'ddd'}),), TestCSVable, True)
    assert result == '\r\n\r\n', repr(result)
    TestCSVable.CSV_FIELDS = {'aaa': 'bbb', 'ccc': 'ddd'}
    result = checkCSVObjectWriter((), TestCSVable)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), TestCSVable, True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable)
    assert result == ',\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable(),), TestCSVable, True)
    assert result == 'aaa,ccc\r\n,\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'bbb': 'xxx', 'ddd': 'yyy', 'fff': 'zzz'}),), TestCSVable)
    assert result == 'xxx,yyy\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'bbb': 'xxx', 'ddd': 'yyy', 'fff': 'zzz'}),), TestCSVable, True)
    assert result == 'aaa,ccc\r\nxxx,yyy\r\n', repr(result)
    TestCSVable.CSV_FIELDS = OrderedDict((('ccc', 'ddd'), ('aaa', 'bbb')))
    result = checkCSVObjectWriter((), TestCSVable)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), TestCSVable, True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'bbb': 'xxx', 'ddd': 'yyy', 'fff': 'zzz'}),), TestCSVable)
    assert result == 'yyy,xxx\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'bbb': 'xxx', 'ddd': 'yyy', 'fff': 'zzz'}),), TestCSVable, True)
    assert result == 'ccc,aaa\r\nyyy,xxx\r\n', repr(result)
    TestCSVable.CSV_FIELDS = ['ccc', 'aaa']
    result = checkCSVObjectWriter((), TestCSVable)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((), TestCSVable, True)
    assert result == '', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'xxx', 'bbb': 'yyy', 'ccc': 'zzz'}),), TestCSVable)
    assert result == 'zzz,xxx\r\n', repr(result)
    result = checkCSVObjectWriter((TestCSVable({'aaa': 'xxx', 'bbb': 'yyy', 'ccc': 'zzz'}),), TestCSVable, True)
    assert result == 'ccc,aaa\r\nzzz,xxx\r\n', repr(result)

def main():
    """Runs build-in tests."""
    testCSVableParser()
    testCSVObjectReader()
    testCSVObjectWriter()
    print "Tests OK"

if __name__ == '__main__':
    main()

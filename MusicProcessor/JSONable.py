#!/usr/bin/python
#
# JSONable.py
#
# A class that can be easily constructed from a JSON object.
#
from collections import OrderedDict
from json import dumps as jsonDumps

class JSONable(object):
    IGNORE_FIELDS = ()
    OUTPUT_FIELDS = ()

    def __init__(self, **kwargs):
        self._parseFields(**kwargs)

    @staticmethod
    def _parseValue(value):
        if isinstance(value, unicode):
            try:
                value = str(value)
            except UnicodeError:
                pass
        elif isinstance(value, list):
            value = tuple(JSONable._parseValue(element) for element in value) # pylint: disable=all
        elif isinstance(value, dict):
            value = JSONable(**value)
        return value

    def _processFields(self):
        pass

    def _parseFields(self, **kwargs):
        for (field, value) in kwargs.iteritems():
            if field in self.IGNORE_FIELDS:
                continue
            field = str(field)
            field = field[0].lower() + field[1:]
            setattr(self, field, self._parseValue(value))
        self._processFields()

    def getFields(self, isOutput = False):
        if isOutput: # pylint: disable=all
            return OrderedDict((field, getattr(self, field)) for field in self.OUTPUT_FIELDS)
        else:
            return dict((field, value) for (field, value) in self.__dict__.iteritems() if field[0].islower() and field not in self.IGNORE_FIELDS)

    def json(self, isOutput = False, **kwargs):
        return jsonDumps(self.getFields(isOutput), **kwargs)

    def __repr__(self):
        return repr(self.getFields())

    @classmethod
    def fromIterable(cls, iterable, *args):
        return (cls(*args, **i) for i in iterable)

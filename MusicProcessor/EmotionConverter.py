#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Emotions Converter library for "Dark Tower: All Hail" LARP.
#

from itertools import chain

try:
    from pytils.translit import translify
except ImportError, ex:
    raise ImportError("%s: %s\n\nPlease install pytils v0.3 or later: https://pypi.python.org/pypi/pytils\n" % (ex.__class__.__name__, ex))

try:
    from unidecode import unidecode
except ImportError, ex:
    raise ImportError("%s: %s\n\nPlease install Unidecode v0.04.16 or later: https://pypi.python.org/pypi/Unidecode/\n" % (ex.__class__.__name__, ex))

TRANSLIFY_PATCHES = {
    u'Ё': u'Е',
    u'ё': u'е',
    u'и\u0306': u'й' # й on Mac
}

EMOTIONS_REPLACEMENTS = {
    'bless': ('blas', 'blass', 'bles', 'blis', 'bliss', 'radost', 'radost\'', u'радость', 'blagost', 'blagost\'', u'благость', 'blagaya', u'благая'),
    'fate': ('fait', 'faith', 'fathe', 'sudba', 'sud\'ba', u'судьба'),
    'fear': ('feer', 'fiar', 'faer', 'strah'),
    'road': ('rod', 'raod', 'roud', 'doroga', u'дорога')
}

EMOTION_PATCHES = dict(chain.from_iterable(((source, replacement) for source in sources) for (replacement, sources) in EMOTIONS_REPLACEMENTS.iteritems()))

def convert(s):
    for (f, t) in TRANSLIFY_PATCHES.iteritems():
        s = s.replace(f, t)
    ret = []
    for c in s:
        try:
            c = translify(c)
        except ValueError:
            c = unidecode(c)
        ret.append(c)
    return ''.join(ret)

def convertTitle(s):
    return convert(s.strip())

def convertEmotion(s):
    e = convertTitle(s).lower()
    return EMOTION_PATCHES.get(e, e).upper()

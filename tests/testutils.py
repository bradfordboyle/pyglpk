"""Contains material useful for all the different PyGLPK test modules."""

import sys, os.path
newpath = os.path.dirname(os.path.dirname(__file__))
sys.path.insert(0, newpath)

# Of course, including the glpk module is useful.
from glpk import *
import unittest

# Testing functions (excepting those testing out) generally shall not
# benefit from having output.
env.term_on = False

# These are for backward compatibility with old Python versions, which
# did not necessarily contain either the basic set type, nor the
# "sorted" convenience method.
try:
    a = set()
except NameError:
    import sets
    set = sets.Set
try:
    sorted([2,1])
except NameError:
    def sorted(seq):
        """Given an iterable, return a list with returned vals sorted."""
        s = list(seq)
        s.sort()
        return s

def extractSuite(obj):
    """Returns the test suite for all test cases within an object.

    If the obj has an attribute 'suite,' it will return it if it is of
    type TestSuite, or call it otherwise and return the result.  This
    can allow modules some flexibility

    If this attribute is absent or is neither a suite nor a callable
    object, this function will finds attributes within the argument
    that are either unittest.TestCase or TestSuite objects, and puts
    them into a suite object which is returned."""
    tload = unittest.defaultTestLoader
    # Find the attribute suite if it exists, and return it if it is a
    # test suite or call it.
    if hasattr(obj, 'suite'):
        s = obj.suite
        if issubclass(type(s), unittest.TestSuite): return s
        if callable(s): return s()
    # Build the test suite.
    the_suite = unittest.TestSuite()
    for v in vars(obj).values():
        if type(v)==type and issubclass(v, unittest.TestCase):
            subsuite = tload.loadTestsFromTestCase(v)
            the_suite.addTests(subsuite)
    return the_suite

def testsRunner(obj):
    """Runs the suite retrieved from calling suite(obj).

    This will run this module's suite function upon the passed in
    object, and run the resulting test suite upon a
    unittest.TextTestRunner."""
    print obj.__name__, ':', obj.__doc__
    return unittest.TextTestRunner(verbosity=2).run(extractSuite(obj))

# The runner convenience class.
class Runner:
    """Run arbitrary expressions as function calls.

    This enables subclasses to retrieve a callable object that, when
    run, runs arbitrary expressions.  The reason this is specified as
    a class rather than a standalone function is because this way one
    may refer to variables local in scope to the calling class and the
    class function from which they are called.

    The primary purpose of this is to enable easy use of the
    assertRaises method in test cases, which expects."""
    def runner(self, statement):
        """Returns a callable object for an arbitrary expression.

        This functions accepts a string which is a valid Python
        expresison, and returns a callable that will evaluate that
        Python expression when called.  For example:

        run = self.runner('a = 5; print a')

        When one calls run(), this will evaluate this expression."""
        eval(compile(statement, 'foobar.py', 'single'))

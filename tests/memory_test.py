"""Tests for proper memory deallocation of objects."""

import glpk
import weakref
import gc
import unittest


class GarbageCollectionTestCase(unittest.TestCase):
    """A garbage collection test case.

    In a nutshell, this test case provides an easy method to help
    identify potential memory leaks.

    This test case class wraps every test method with code that
    defines a list of weak references.  The test method may add items
    to this list with the reg registration method.  The idea is that
    registered items are those that the user expects to be freed.  The
    end of the wrapper code forces the garbage collector to collect,
    and then ensures that all the weak references added to the weak
    reference list are currently invalid."""
    def __init__(self, name=''):
        unittest.TestCase.__init__(self, name)
        oldfunc = getattr(self, name)

        class tuplesubclass(list):
            pass
        self.tuplesubclass = tuplesubclass

        # "Wrap" the test function function with this replacement.
        def replacement():
            self.weakrefs = []
            oldfunc()                      # Call the original callable obj.
            gc.collect()
            bad_objs = []
            for wr in self.weakrefs:
                ob = wr()
                if ob is not None:
                    bad_objs.append(ob)
            self.assertEqual(len(bad_objs), 0,  # Run final check.
                             'These objs not freed: %s' % bad_objs)
        replacement.__doc__ = oldfunc.__doc__
        setattr(self, name, replacement)

    def reg(self, ob):
        """The object registering function.

        This appends a weak reference to the object."""
        self.weakrefs.append(weakref.ref(ob))
        return ob

    def testLPCreation(self):
        """Test simple creation of a linear program."""
        reg = self.reg
        reg(glpk.LPX())

    def testLPDefinition(self):
        """Test creation of many objects."""
        reg = self.reg
        lp = reg(glpk.LPX())
        lp.name = 'some name'
        reg(lp.rows).add(2)
        reg(lp.cols).add(5)
        reg(lp.rows[0]).matrix = [1, 2, 3, 4, 5]
        reg(lp.rows[1]).name = 'second row'
        for c in reg(iter(reg(lp.cols))):
            reg(c).bounds = 1, 2
        reg(reg(lp.cols)[0]).name
        reg(lp.obj)[:] = [6, 7, 8, 9, 10]
        sum([v for v in reg(iter(reg(lp.obj)))])

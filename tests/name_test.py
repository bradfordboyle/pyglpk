"""Tests for naming objects with .name attributes."""
import unittest
import glpk


class NameTestCase:
    """Constains test set methods for setting names.

    Note that these tests are not a unit test test case, but they can
    be inhereted by a test case to test the 'nameability' of various
    objects within GLPK.

    In order to use this series of tests, one would assign the
    object's attribute 'nameable' in the setUp() method to the object
    one should treat as having an assignable 'name' attribute."""
    def testNameInitiallyNone(self):
        """Tests that a name is initially None."""
        self.assertEqual(self.nameable.name, None)

    def testNameSet(self):
        """Tests that setting a name sticks."""
        self.nameable.name = 'a simple name'
        self.assertEqual(self.nameable.name, 'a simple name')

    def testNameReset(self):
        """Tests that setting a name unsets the old name."""
        self.nameable.name = 'albatross'
        self.nameable.name = 'new names? '
        self.assertEqual(self.nameable.name, 'new names? ')

    def testNameSetNone(self):
        """Tests unsetting a name by setting it to None."""
        self.nameable.name = 'foo'
        self.nameable.name = None
        self.assertEqual(self.nameable.name, None)

    def testNameDelete(self):
        """Tests unsetting a name by using a del statement."""
        self.nameable.name = 'foo'
        del self.nameable.name
        self.assertEqual(self.nameable.name, None)

    def testNameSetToBadType(self):
        """Tests that setting non-string names raises TypeErrors."""
        def f(newName): self.nameable.name = newName
        self.assertRaises(TypeError, f, 4)
        self.assertRaises(TypeError, f, ['foo'])
        self.assertRaises(TypeError, f, {'foo': 'bar'})


class LPNameTestCase(unittest.TestCase, NameTestCase):
    """Naming tests for the linear program name."""
    def setUp(self):
        self.lp = glpk.LPX()
        self.nameable = self.lp


class ObjectiveNameTestCase(unittest.TestCase, NameTestCase):
    """Naming tests for the objective function name."""
    def setUp(self):
        self.lp = glpk.LPX()
        self.nameable = self.lp.obj


class RowNameTestCase(unittest.TestCase, NameTestCase):
    """Naming tests for a row name."""
    def setUp(self):
        self.lp = glpk.LPX()
        rnew = self.lp.rows.add(1)
        self.nameable = self.lp.rows[rnew]


class ColumnNameTestCase(unittest.TestCase, NameTestCase):
    """Naming tests for a column name."""
    def setUp(self):
        self.lp = glpk.LPX()
        cnew = self.lp.cols.add(1)
        self.nameable = self.lp.cols[cnew]

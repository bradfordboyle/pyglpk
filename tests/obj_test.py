"""Tests for changing the objective function."""

import glpk
import unittest


class ObjectiveDirectionTestCase(unittest.TestCase):
    """Tests whether setting this LP to max/min works.

    Note that these tests do not actually test any sort of
    optimization subject to maximization or minimization.  These tests
    merely check that setting the problem to maximization or
    minimization has the desirable properties of persistence."""
    def setUp(self):
        self.lp = glpk.LPX()

    def testInitiallyTrueOrFalse(self):
        """Tests whether the objective is initially boolean."""
        self.assertEqual(type(self.lp.obj.maximize), bool)

    def testMakeMaximize(self):
        """Tests making the LP a maximiziation problem."""
        self.lp.obj.maximize = True
        self.assertEqual(self.lp.obj.maximize, True)

    def testMakeMinimize(self):
        """Tests making the LP a minimization problem."""
        self.lp.obj.maximize = False
        self.assertEqual(self.lp.obj.maximize, False)


class ObjectiveShiftTestCase(unittest.TestCase):
    """Tests setting the objective's constant shift coefficient."""
    def setUp(self):
        self.lp = glpk.LPX()

    def testInitialShiftAttribute(self):
        """Tests that the initial shift lp.obj.shift is 0."""
        self.assertEqual(self.lp.obj.shift, 0.0)
        self.assertEqual(self.lp.obj[None], 0.0)

    def testSetShiftThroughAttribute(self):
        """Tests our ability to set shift through lp.obj.shift."""
        self.lp.obj.shift = 5.23
        self.assertEqual(self.lp.obj.shift, 5.23)

    def testSetShiftThroughIndex(self):
        """Tests our ability to set shift through lp.obj[None]."""
        self.lp.obj[None] = 2.345
        self.assertEqual(self.lp.obj[None], 2.345)

    def testShiftIndexAndAttributeEquivalent(self):
        """Tests equivalence of lp.obj[None] and lp.obj.shift."""
        self.lp.obj.shift = -12.345
        self.assertEqual(self.lp.obj[None], -12.345)
        self.lp.obj[None] = 3.14159
        self.assertEqual(self.lp.obj.shift, 3.14159)

    def testBadTypeAssignment(self):
        """Tests assigning objects other than a float as shift."""
        with self.assertRaises(TypeError):
            self.lp.obj.shift = None
        with self.assertRaises(TypeError):
            self.lp.obj[None] = "Foo"
        with self.assertRaises(TypeError):
            self.lp.obj.shift = [1]
        with self.assertRaises(TypeError):
            self.lp.obj[None] = {1: 2}


class ObjectiveCoefficientTestCase(unittest.TestCase):
    """Tests setting and indexing objective function coefficients."""
    def setUp(self):
        self.lp = glpk.LPX()
        self.lp.cols.add(5)

    def testObjectiveIterator(self):
        obj_iter = iter(self.lp.obj)
        self.assertEqual(len(obj_iter), 5)
        next(obj_iter)
        self.assertEqual(len(obj_iter), 4)

    def testInitialCoefficients(self):
        """Test that the initial coefficients are 0."""
        for i in range(len(self.lp.cols)):
            self.assertEqual(self.lp.obj[i], 0.0)
        # While we're at it, run the same test with the iterator
        # functionality of the objective.
        self.assertEqual([c for c in self.lp.obj], [0.0]*len(self.lp.cols))
        self.assertEqual(self.lp.obj[:], [0.0]*len(self.lp.cols))

    def testSetSingleCoefficient(self):
        """Test setting a single coefficient."""
        self.lp.obj[1] = 2
        self.assertEqual(self.lp.obj[1], 2.0)
        self.assertEqual(self.lp.obj[:], [0.0, 2.0, 0.0, 0.0, 0.0])

    def testSetTupleIndexToManyCoefficients(self):
        """Tests setting a tuple of coefficients to many coefficients."""
        self.lp.obj[0, -1, 3] = 3.14159, -20, 8
        self.assertEqual(self.lp.obj[:], [3.14159, 0.0, 0.0, 8.0, -20])
        self.assertEqual(self.lp.obj[0, -1, 3], [3.14159, -20, 8])
        self.assertEqual(self.lp.obj[0, None], [3.14159, self.lp.obj.shift])

    def testSetTupleIndexToSingleCoefficient(self):
        """Tests setting a tuple of coefficients to a single coefficient."""
        self.lp.obj[0, -1, 3] = 2.3
        self.assertEqual(self.lp.obj[:], [2.3, 0.0, 0.0, 2.3, 2.3])

    def testSetSliceIndexToManyCoefficients(self):
        """Tests setting a slice of coefficients to many coefficients."""
        self.lp.obj[1::2] = 1, 2
        self.assertEqual(self.lp.obj[:], [0.0, 1.0, 0.0, 2.0, 0.0])

    def testSetSliceIndexToSingleCoefficient(self):
        """Tests setting a slice of coefficients to a single coefficient."""
        self.lp.obj[1::2] = 0.5
        self.assertEqual(self.lp.obj[:], [0.0, 0.5, 0.0, 0.5, 0.0])

    def testStringIndex(self):
        """Tests setting objective coefficients by col name."""
        for c in self.lp.cols:
            c.name = 'x%i' % c.index
        # Test single value assignment.
        self.lp.obj['x1'] = 3.1
        self.assertEqual(self.lp.obj['x1'], 3.1)
        self.assertEqual(self.lp.obj[:], [0.0, 3.1, 0.0, 0.0, 0.0])
        # Test tuple value assignment.
        self.lp.obj['x1', 3, 'x0'] = 123, 456, 789
        self.assertEqual(self.lp.obj[:], [789, 123, 0.0, 456, 0.0])

    def testSetSingleCoefficientBadType(self):
        """Test setting a single coefficient with a bad type."""
        with self.assertRaises(TypeError):
            self.lp.obj[1] = None
        with self.assertRaises(TypeError):
            self.lp.obj[3] = [2]
        with self.assertRaises(TypeError):
            self.lp.obj[-1] = {1: 2}

    def testSetMultipleCoefficientBadType(self):
        """Tests setting multiple coefficients with a bad type."""
        with self.assertRaises(TypeError):
            self.lp.obj[0, 1] = 2, None
        with self.assertRaises(TypeError):
            self.lp.obj[-2:] = 2, [1]
        with self.assertRaises(TypeError):
            self.lp.obj[-2:] = self

    def testSetBadNumberOfCoefficientsToTuple(self):
        """Test a mismatch between assignees and assignments."""
        with self.assertRaises(ValueError):
            self.lp.obj[1, 2, 3] = 4, 5, 6, 7
        with self.assertRaises(ValueError):
            self.lp.obj[1, 2, 3] = 4, 5
        # Tuple indexing for good measure.
        with self.assertRaises(ValueError):
            self.lp.obj[-2:] = 1, 4, 5

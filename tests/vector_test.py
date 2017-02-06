"""Tests for operations upon LP rows and columns."""


from glpk import LPX, env
import unittest


class AddVectorTestCase:
    """Collection of tests for adding rows or columns."""
    def testVectorIsInitiallyEmpty(self):
        """Test that vector collection is initially empty."""
        self.assertEqual(len(self.vc), 0)

    def testAddVector(self):
        """Tests adding a single vector."""
        self.vc.add(1)
        self.assertEqual(len(self.vc), 1)

    def testFirstVectorIndexedAtZero(self):
        """Tests the first vector added has index 0."""
        vec_index = self.vc.add(1)
        self.assertEqual(vec_index, 0)

    def testAddMultipleVectors(self):
        """Tests adding multiple vectors."""
        vec_index = self.vc.add(6)
        self.assertEqual(vec_index, 0)
        self.assertEqual(len(self.vc), 6)

    def testMultipleAddMultipleVectors(self):
        """Tests multiple additions of multiple vectors."""
        num_to_add, running_sum = [1, 5, 3, 3, 3], 0
        for add_now in num_to_add:
            vec_index = self.vc.add(add_now)
            self.assertEqual(vec_index, running_sum)
            running_sum += add_now
            self.assertEqual(len(self.vc), running_sum)

    def testNonpositiveAdd(self):
        """Tests for errors when adding non-postive number of vectors."""
        self.assertRaises(ValueError, self.vc.add, 0)
        self.assertRaises(ValueError, self.vc.add, -1)
        self.assertRaises(ValueError, self.vc.add, -100)

    def testBadAddType(self):
        """Tests for errors when adding non-int number of vectors."""
        self.assertRaises(TypeError, self.vc.add, (2,))
        self.assertRaises(TypeError, self.vc.add, [5, 4])
        self.assertRaises(TypeError, self.vc.add, 'foobar')


class IndexVectorTestCase:
    """Collection of tests for indexing/accessing rows and columns."""
    def testEmptySingleIntegerLookup(self):
        """Tests integer indexing of an empty vector collection."""
        for i in (0, -4, 5):
            def f(): self.vc[i]
            self.assertRaises(IndexError, f)

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testEmptySingleStringLookup(self):
        """Tests string indexing of an empty vector collection."""

        def f(key):
            self.vc[key]
        self.assertRaises(KeyError, f, "foo")

    def testEmptyTupleIntegerLookup(self):
        """Tests integer-tuple indexing of an empty vector collection."""
        def f(): self.vc[0, 2]
        self.assertRaises(IndexError, f)

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testEmptyTupleStringLookup(self):
        """Tests string-tuple indexing of an empty vector collection."""

        def f():
            self.vc["foo", "bar"]
        self.assertRaises(KeyError, f)

    def testEmptyTupleMixedLookup(self):
        """Tests mixed-tuple indexing of an empty vector collection."""
        def f(): self.vc[2, "foo"]
        self.assertRaises(LookupError, f)

    def testEmptySliceLookup(self):
        """Tests slice lookups on empty vector collections."""
        self.assertEqual(len(self.vc[4:9]), 0)
        self.assertEqual(len(self.vc[:-1]), 0)
        self.assertEqual(len(self.vc[-3:]), 0)

    def testIntegerIndexingOfSingleVector(self):
        """Tests indexing a single vector with an int."""
        self.vc.add(1)
        vector = self.vc[0]
        self.assertEqual(vector.index, 0)

    def testIntegerIndexingOfMultipleVector(self):
        """Tests indexing many vectors with ints."""
        self.vc.add(6)
        v0, v2, v5 = self.vc[0], self.vc[2], self.vc[5]
        self.assertEqual(v0.index, 0)
        self.assertEqual(v2.index, 2)
        self.assertEqual(v5.index, 5)

    def testNegativeIntegerIndexingOfMultipleVectors(self):
        """Tests indexing vectors with negative ints."""
        self.vc.add(9)
        v8, v2, v4 = self.vc[-1], self.vc[-7], self.vc[-5]
        self.assertEqual(v8.index, 8)
        self.assertEqual(v2.index, 2)
        self.assertEqual(v4.index, 4)

    def testIntegerTupleIndexingOfMultipleVector(self):
        """Tests indexing many vectors with int tuples."""
        self.vc.add(6)
        self.assertEqual([v.index for v in self.vc[0, 5, 2]], [0, 5, 2])

    def testNegativeIntegerTupleIndexingOfMultipleVectors(self):
        """Tests indexing vectors with negative int tuples."""
        self.vc.add(9)
        self.assertEqual([v.index for v in self.vc[-1, -7, -5]], [8, 2, 4])

    def testAnyIntegerTupleIndexingOfMultipleVectors(self):
        """Tests indexing vectors with positive/negative int tuples."""
        self.vc.add(11)
        self.assertEqual([v.index for v in self.vc[-1, 7, -4]], [10, 7, 7])

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testStringIndexingOfSingleVector(self):
        """Tests indexing a single vector with a string."""
        self.vc.add(5)
        self.vc[2].name = 'foo'
        self.assertEqual(self.vc['foo'].index, 2)
        self.assertEqual(self.vc['foo'].name, 'foo')

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testStringMembershipCheck(self):
        """Tests the 'in' operator for string indices."""
        self.vc.add(5)
        self.assertFalse('foo' in self.vc)
        self.vc[2].name = 'foo'
        self.assertTrue('foo' in self.vc)
        self.assertFalse('bar' in self.vc)

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testStringTupleIndexingOfMultipleVector(self):
        """Tests indexing multiple vectors with a string tuple."""
        self.vc.add(5)
        self.vc[2].name = 'foo'
        self.vc[4].name = 'bar'
        self.vc[0].name = 'blammo'
        self.assertEqual([v.index for v in self.vc['foo', 'blammo', 'bar']],
                         [2, 0, 4])
        self.assertEqual([v.name for v in self.vc['bar', 'foo', 'blammo']],
                         ['bar', 'foo', 'blammo'])

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testStringNameReset(self):
        """Test that a name is no longer a valid index once set/reset."""
        self.vc.add(5)
        self.vc[2].name = 'foo'
        self.assertEqual(self.vc['foo'].name, 'foo')

        def f(old, new):
            self.vc[old].name = new
        f('foo', 'bar')  # First, set the name to something new.
        self.assertRaises(KeyError, f, 'foo', 'bar')  # Lookup on old name.
        del self.vc['bar'].name
        self.assertRaises(KeyError, f, 'bar', 'foo')  # Lookup on deleted name.

    def testSliceIndexingOfVectors(self):
        """Tests indexing vectors by slice."""
        to_add = 37
        i = list(range(to_add))
        self.vc.add(to_add)
        self.assertEqual([v.index for v in self.vc[:5]], i[:5])
        self.assertEqual([v.index for v in self.vc[:-4]], i[:-4])
        self.assertEqual([v.index for v in self.vc[-4:]], i[-4:])
        self.assertEqual([v.index for v in self.vc[::-1]], i[::-1])
        self.assertEqual([v.index for v in self.vc[4:29]], i[4:29])
        self.assertEqual([v.index for v in self.vc[4:2]], i[4:2])
        self.assertEqual([v.index for v in self.vc[4:30:3]], i[4:30:3])

    def testBadTypeIndexingOfVectors(self):
        """Tests indexing with non-integers/strings gives type errors."""
        self.vc.add(1)

        def f(): self.vc[{1: 2, 3: 4}]
        self.assertRaises(TypeError, f)

    def testBadTypeMultipleIndexingOfVectors(self):
        """Tests bad type indices mixed with good indices."""
        self.vc.add(5)

        def f():
            self.vc[-1, (1, 2, 3, 4), 2]
        self.assertRaises(TypeError, f)

    def testIteratorAccessToVectors(self):
        """Tests that the vector collection may be iterated upon."""
        to_add = 78
        self.vc.add(to_add)
        self.assertEqual([v.index for v in self.vc], list(range(to_add)))

    def testIteratorWriteAccessToVectors(self):
        """Tests that the vectors from iteration are mutable."""
        to_add = 12
        self.vc.add(to_add)
        for vec in self.vc:
            vec.name = 'x%d' % vec.index
        for vec in self.vc:
            self.assertEqual(vec.name, 'x%d' % vec.index)
        self.assertEqual(self.vc[3].name, 'x3')
        if env.version >= (4, 7):
            self.assertEqual(self.vc['x10'].index, 10)


class DeleteVectorTestCase:
    """Tests deleting vectors from the vector collection."""
    def moreSetUp(self):
        self.num_vecs = 5
        self.vc.add(self.num_vecs)
        for v in self.vc:  # Name them.
            v.name = 'x%d' % v.index

    @unittest.skipIf(env.version < (4, 7), 'TODO')
    def testDeleteOneVectorDecreasesLength(self):
        """Tests length of the vector collection after deleting one."""
        del self.vc[3]
        self.assertEqual(len(self.vc), self.num_vecs-1)
        self.assertEqual(self.vc[3].name, 'x4')
        self.assertTrue('x3' not in self.vc)

    def testDeleteTupleVectorDecreasesLength(self):
        """Tests deleting columns indexed by tuple."""
        del self.vc[4, -4, 0]
        self.assertEqual(len(self.vc), self.num_vecs-3)
        self.assertEqual(self.vc[0].name, 'x2')
        self.assertEqual(self.vc[1].name, 'x3')

    def testDeleteSimpleSliceVectorDecreasesLength(self):
        """Tests deleting columns indexed by slice."""
        del self.vc[:2]
        self.assertEqual(len(self.vc), self.num_vecs-2)
        self.assertEqual([v.name for v in self.vc], ['x2', 'x3', 'x4'])
        if env.version >= (4, 7):
            self.assertTrue('x0' not in self.vc)
            self.assertTrue('x1' not in self.vc)
        del self.vc[:]
        self.assertEqual(len(self.vc), 0)

    def testDeleteExtendedSliceVectorDecreasesLength(self):
        """Tests deleting columns indexed by an extended slice."""
        del self.vc[::2]
        self.assertEqual(len(self.vc), self.num_vecs // 2)
        self.assertEqual([v.name for v in self.vc],
                         ['x%d' % i for i in range(1, self.num_vecs, 2)])
        if env.version >= (4, 7):
            for i in range(0, self.num_vecs, 2):
                self.assertTrue(('x%d' % i) not in self.vc)

    def testDeleteVectorInvalidation(self):
        """Tests Bar invalidation once its index becomes out of bounds."""
        self.thebar = self.vc[-1]
        self.thebar.name  # This should have no problems.
        del self.vc[::2]
        with self.assertRaises(RuntimeError):
            self.thebar.name
        with self.assertRaises(RuntimeError):
            self.thebar.name = 5
        with self.assertRaises(RuntimeError):
            self.thebar.matrix
        del self.thebar


class BoundVectorTestCase:
    """Tests the setting of bounds.

    Note that this merely tests that once set, the bounds on the
    vectors are stored properly, not that the bounds are respected
    when it comes time to do the actual optimization."""
    def moreSetUp(self):
        self.vec = self.vc[self.vc.add(1)]

    def testInitiallyNoBounds(self):
        """Test that initial bounds are what we expect."""
        # The GLPK default (at least in version 4.14) is that row
        # variables are initially unbounded, and column variables are
        # constrained to equal 0.
        if self.vec.isrow:
            self.assertEqual(self.vec.bounds, (None, None))
        else:
            self.assertEqual(self.vec.bounds, (0.0, 0.0))

    def testSetUnbounded(self):
        """Tests setting a variable as unbounded."""
        self.vec.bounds = None
        self.assertEqual(self.vec.bounds, (None, None))

    def testSetEqualityBounds(self):
        """Tests setting equality bounds."""
        for num in [3, -2, 3.145159, 1234567.89]:
            self.vec.bounds = num
            self.assertEqual(self.vec.bounds, (num, num))

    def testSetLowerBounds(self):
        """Tests setting a lower bound."""
        for num in [3, -2, 3.145159, 1234567.89]:
            self.vec.bounds = num, None
            self.assertEqual(self.vec.bounds, (num, None))

    def testSetUpperBounds(self):
        """Tests setting an upper bound."""
        for num in [3, -2, 3.14159, 1234567.89]:
            self.vec.bounds = None, num
            self.assertEqual(self.vec.bounds, (None, num))

    def testDeleteBounds(self):
        """Tests setting a vector as unbounded through deletion."""
        self.vec.bounds = 3
        self.assertEqual(self.vec.bounds, (3, 3))
        del self.vec.bounds
        self.assertEqual(self.vec.bounds, (None, None))

    def testSetRangeBounds(self):
        """Tests setting a range bound."""
        bounds = [(2, 4), (-1, 0), (3.14159, 3.5), (3.3, 3.3), (-100, 300)]
        for bound in bounds:
            self.vec.bounds = bound
            self.assertEqual(self.vec.bounds, bound)

    def testManyBounds(self):
        """Tests setting many different types of bounds."""
        bounds = [(2, 4), None, (-1, 0), (4.5, None), 5, 123.4,
                  (3.14159, 3.5), (3.3, 3.3), None, (-100, 300),
                  -8, (1, None), (None, 7)]
        for bound in bounds:
            self.vec.bounds = bound
            if type(bound) == tuple:
                self.assertEqual(self.vec.bounds, bound)
            else:
                self.assertEqual(self.vec.bounds, (bound, bound))

    def testEmptyBounds(self):
        """Tests lower bounds greater than upper bounds is an error."""
        with self.assertRaises(ValueError):
            self.vec.bounds = (5, 4)

    def testBadTupleNumberBounds(self):
        """Tests that non-2-element-tuples are unacceptable bounds."""
        with self.assertRaises(TypeError):
            self.vec.bounds = (4,)
        with self.assertRaises(TypeError):
            self.vec.bounds = 2, 4, 5
        with self.assertRaises(TypeError):
            self.vec.bounds = None, 2, 3, 3

    def testBadTypeBounds(self):
        """Tests that bad types assigned as bounds are caught."""
        with self.assertRaises(TypeError):
            self.vec.bounds = "Hi!"
        with self.assertRaises(TypeError):
            self.vec.bounds = {1: 2}

    def testGoodBoundsKeptAfterBadSetAttempts(self):
        """Tests that bad bound sets do not overwrite old bounds."""
        self.vec.bounds = 3, 5
        try:
            self.vec.bounds = 4, 6, None
        except TypeError:
            pass
        self.assertEqual(self.vec.bounds, (3, 5))
        try:
            self.vec.bounds = 6, 4
        except ValueError:
            pass
        self.assertEqual(self.vec.bounds, (3, 5))


class ScaleVectorTestCase:
    """Tests setting row and column scaling."""
    def moreSetUp(self):
        self.num_vecs = 5
        self.vc.add(self.num_vecs)

    def testSetScaling(self):
        """Simple test to see that scaling factors are set."""
        self.vc[0].scale = 2
        self.vc[1].scale = 3.14159
        self.assertAlmostEqual(self.vc[0].scale, 2)
        self.assertAlmostEqual(self.vc[1].scale, 3.14159)

    def testNonpositiveScaling(self):
        """Test whether non-positive scale factors throw exceptions."""
        with self.assertRaises(ValueError):
            self.vc[0].scale = -1.0
        with self.assertRaises(ValueError):
            self.vc[1].scale = 0.0

    def testBadScaleType(self):
        """Tests whether non-numeric scale factors throw exceptions."""
        self.vc[0].scale = 3.14159
        with self.assertRaises(TypeError):
            self.vc[0].scale = complex(2, 2)
        with self.assertRaises(TypeError):
            self.vc[1].scale = None
        with self.assertRaises(TypeError):
            self.vc[2].scale = {}
        with self.assertRaises(TypeError):
            self.vc[3].scale = "foo"
        self.assertAlmostEqual(self.vc[0].scale, 3.14159)

    def testDeleteScale(self):
        """Testss whether deleting a scale factor throws exceptions."""
        with self.assertRaises(AttributeError):
            del self.vc[0].scale


class RowTestCase(unittest.TestCase):
    def moreSetUp(self): pass

    def setUp(self):
        self.lp = LPX()
        self.vc = self.lp.rows  # Vector collection is the row list.
        self.moreSetUp()


class ColumnTestCase(unittest.TestCase):
    def moreSetUp(self): pass

    def setUp(self):
        self.lp = LPX()
        self.vc = self.lp.cols  # Vector collection is the column list.
        self.moreSetUp()


class AddRowTestCase(AddVectorTestCase, RowTestCase):
    pass


class AddColumnTestCase(AddVectorTestCase, ColumnTestCase):
    pass


class IndexRowTestCase(IndexVectorTestCase, RowTestCase):
    pass


class IndexColumnTestCase(IndexVectorTestCase, ColumnTestCase):
    pass


class DeleteRowTestCase(DeleteVectorTestCase, RowTestCase):
    pass


class DeleteColumnTestCase(DeleteVectorTestCase, ColumnTestCase):
    pass


class BoundRowTestCase(BoundVectorTestCase, RowTestCase):
    pass


class BoundColumnTestCase(BoundVectorTestCase, ColumnTestCase):
    pass


class ScaleRowTestCase(ScaleVectorTestCase, RowTestCase):
    pass


class ScaleColumnTestCase(ScaleVectorTestCase, ColumnTestCase):
    pass


class ComparisonTestCase(unittest.TestCase):
    """Test the rich comparison operators on vectors."""
    def setUp(self):
        self.lp = LPX()
        self.r = self.lp.rows
        self.c = self.lp.cols
        self.r.add(5)
        self.c.add(4)

    def testEquality(self):
        """Simple tests of equality of the same vectors."""
        self.assertEqual(self.r[2], self.r[2])
        self.assertEqual(self.c[-1], self.c[3])

    def testInequality(self):
        """Simple tests of inequality of different vectors."""
        self.assertNotEqual(self.r[2], self.r[3])
        self.assertNotEqual(self.c[0], self.c[2])

    def testComparisonSameCollection(self):
        """Test that vectors in the same collection are ordered by index."""
        self.assertTrue(self.r[0] < self.r[1])
        self.assertTrue(self.r[2] > self.r[1])
        self.assertTrue(self.c[2] > self.c[1])
        self.assertTrue(self.c[2] < self.c[-1])

    def testComparisonDifferentCollection(self):
        """Test vector comparions in different collections."""
        a, b = self.r, self.c
        if b < a:
            a, b = b, a
        # A should be the lower bar collection.
        self.assertTrue(a[0] < b[0])
        self.assertTrue(b[1] > a[3])
        self.assertTrue(a[1] < b[0])

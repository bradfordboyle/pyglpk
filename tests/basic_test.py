"""Tests for basic functionality"""

import tempfile
import sys
import unittest

from glpk import LPX

MODEL = '''
var x >= 0, <= 1;
var y >= 0, <= 1;

param a;
param b;

maximize sum: x + y;
s.t. foo: a * x + y <= 1;

end;
'''

DATA = '''
param b := 0.5;
'''


class BarColTests(unittest.TestCase):

    def setUp(self):
        self.lp = LPX()
        self.lp.cols.add(5)
        self.lp.rows.add(5)

    def testBarColIter(self):
        row_iter = iter(self.lp.rows)
        self.assertEqual(len(row_iter), 5)
        next(row_iter)
        self.assertEqual(len(row_iter), 4)

    @unittest.skipIf(
        sys.version_info > (3,),
        "rich comparison for BarCol is under development"
    )
    def testBarColRichCompare(self):
        cols = self.lp.cols
        rows = self.lp.rows

        self.assertFalse(cols == 1)
        self.assertTrue(cols != 1)
        self.assertFalse(cols < 1)
        self.assertFalse(cols <= 1)
        self.assertTrue(cols > 1)
        self.assertTrue(cols >= 1)

        self.assertTrue(cols == cols)
        self.assertFalse(cols == rows)
        self.assertFalse(cols != cols)
        self.assertTrue(cols != rows)
        self.assertFalse(cols < cols)
        self.assertTrue(cols <= cols)
        self.assertFalse(cols > cols)
        self.assertTrue(cols >= cols)

        # these comparison's depend on the id of the object which changes order
        # between runs
        self.assertEqual(cols < rows, cols <= rows)
        self.assertNotEqual(cols <= rows, cols >= rows)
        self.assertEqual(cols > rows, cols >= rows)

    def testBarColDelete(self):
        with self.assertRaises(KeyError) as cm:
            del self.lp.cols[0, 'x']
        self.assertIn("col named 'x' does not exist", str(cm.exception))
        with self.assertRaises(ValueError) as cm:
            del self.lp.cols[0, 1, 0]
        self.assertIn("duplicate index detected", str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            del self.lp.cols[0.1]
        self.assertIn(
            "bad index type for row/col collection",
            str(cm.exception)
        )

    def testBarColAdd(self):
        self.lp.rows.add(3)
        self.lp.cols[0] = [2, 4, 6]
        self.assertTrue(len(self.lp.matrix), 3)
        self.assertEqual(self.lp.matrix[1], (1, 0, 4.0))
        with self.assertRaises(TypeError) as cm:
            self.lp.cols[0.1] = [2, 4, 6]
        self.assertIn(
            "bad index type for row/col collection",
            str(cm.exception)
        )

    def testBarColStr(self):
        lp_id = id(self.lp)
        self.assertIn(
            "<glpk.BarCollection, cols of glpk.LPX 0x{0:02x}".format(lp_id),
            str(self.lp.cols)
        )

    def testBarRichCompare(self):
        c0 = self.lp.cols[0]
        c1 = self.lp.cols[1]

        self.assertFalse(c0 == 0)
        self.assertFalse(c0 != 0)

        self.assertEqual(NotImplemented, c0.__lt__(1))

        self.assertTrue(c0 <= c1)
        self.assertFalse(c0 >= c1)

    def testBarStr(self):
        lp_id = id(self.lp)
        self.assertIn(
            "glpk.Bar, col 0 of glpk.LPX 0x{0:02x}".format(lp_id),
            str(self.lp.cols[0])
        )

    def testBarSetName(self):
        self.lp.cols[0].name = 'a' * 255
        with self.assertRaises(ValueError) as cm:
            self.lp.cols[0].name = 'a' * 256
        self.assertIn("name may be at most 255 chars", str(cm.exception))

    def testBarValid(self):
        c = self.lp.cols[-1]
        self.assertTrue(c.valid)
        del(self.lp.cols[-1])
        self.assertFalse(c.valid)

    def testBarIsCol(self):
        c = self.lp.cols[0]
        r = self.lp.rows[0]
        self.assertTrue(c.iscol)
        self.assertFalse(r.iscol)

    def testBarStatus(self):
        c = self.lp.cols[0]

        with self.assertRaises(ValueError) as cm:
            c.status = 'xxx'
        self.assertIn(
            "status strings must be length 2",
            str(cm.exception)
        )

        with self.assertRaises(ValueError) as cm:
            c.status = 'xx'
        self.assertIn(
            "status string value 'xx' unrecognized",
            str(cm.exception)
        )

        c.status = 'bs'
        self.assertEqual(c.status, 'bs')
        c.status = 'ns'
        self.assertEqual(c.status, 'ns')

        c.bounds = None, None
        c.status = 'nf'
        self.assertEqual(c.status, 'nf')

        c.bounds = 0, 1
        c.status = 'nl'
        self.assertEqual(c.status, 'nl')
        c.status = 'nu'
        self.assertEqual(c.status, 'nu')

        with self.assertRaises(AttributeError) as cm:
            del(c.status)
        self.assertIn("cannot delete status", str(cm.exception))

    def testBarKind(self):
        c = self.lp.cols[0]

        self.assertEqual(c.kind, float)
        c.kind = int
        self.assertEqual(c.kind, int)
        c.kind = bool
        self.assertEqual(c.kind, bool)

        r = self.lp.rows[0]
        with self.assertRaises(ValueError) as cm:
            r.kind = bool
        self.assertIn("row variables cannot be binary", str(cm.exception))

    def testBarSpecVars(self):
        c = self.lp.cols[0]
        self.assertEqual(c.primal_s, 0.0)
        self.assertEqual(c.dual_s, 0.0)

        # TODO: need to actually test w/ MIP
        with self.assertRaises(TypeError) as cm:
            c.value_m
        self.assertIn(
            "MIP values require mixed integer problem",
            str(cm.exception)
        )

    def testBarSetMatrix(self):
        with self.assertRaises(ValueError) as cm:
            self.lp.cols[0].matrix = [(1, 2, 3)]
        self.assertIn(
            'vector entry tuple has length 3; 2 is required',
            str(cm.exception)
        )


class LpxTests(unittest.TestCase):

    def testInit(self):
        with self.assertRaises(TypeError) as cm:
            LPX(gmp='', mps='')
        self.assertIn(
            'cannot specify multiple data sources',
            str(cm.exception)
        )

        with self.assertRaises(RuntimeError) as cm:
            LPX(gmp=('', None, ''))
        self.assertIn(
            'GMP model reader failed',
            str(cm.exception)
        )

        model = tempfile.NamedTemporaryFile(mode='w')
        model.write(MODEL)
        model.flush()

        data = tempfile.NamedTemporaryFile(mode='w')
        data.write(DATA)
        data.flush()

        with self.assertRaises(RuntimeError) as cm:
            LPX(gmp=(model.name, ''))
        self.assertIn(
            'GMP data reader failed',
            str(cm.exception)
        )

        with self.assertRaises(RuntimeError) as cm:
            LPX(gmp=(model.name, data.name))
        self.assertIn(
            'GMP generator failed',
            str(cm.exception)
        )

    def testLpxStr(self):
        lp = LPX()
        self.assertIn(
            '<glpk.LPX 0-by-0 at 0x{0:02x}>'.format(id(lp)),
            str(lp)
        )

    def testLpxErase(self):
        lp = LPX()
        lp.cols.add(2)
        lp.rows.add(2)
        self.assertEqual(len(lp.cols), 2)
        self.assertEqual(len(lp.rows), 2)
        lp.erase()
        self.assertEqual(len(lp.cols), 0)
        self.assertEqual(len(lp.rows), 0)

    def testLpxSetMatrix(self):
        lp = LPX()
        with self.assertRaises(ValueError) as cm:
            lp.matrix = [(1, 2, 3, 4)]
        self.assertIn(
            'matrix entry tuple has length 4; 3 is required',
            str(cm.exception)
        )

    def testLpxInteger(self):
        lp = LPX()

        with self.assertRaises(RuntimeError) as cm:
            lp.integer()
        self.assertIn(
            'integer solver without presolve requires existing optimal basic '
            'solution',
            str(cm.exception)
        )

        with self.assertRaises(ValueError) as cm:
            lp.integer(ps_tm_lim=-1, presolve=True)
        self.assertIn('ps_tm_lim must be nonnegative', str(cm.exception))

        with self.assertRaises(ValueError) as cm:
            lp.integer(mip_gap=-1, presolve=True)
        self.assertIn('mip_gap must be non-negative', str(cm.exception))

    def testLpxKkt(self):
        lp = LPX()

        with self.assertRaises(RuntimeError) as cm:
            lp.kkt()
        self.assertIn(
            'cannot get KKT when primal or dual basic solution undefined',
            str(cm.exception)
        )

    def testLpxName(self):
        lp = LPX()
        with self.assertRaises(ValueError) as cm:
            lp.name = 'a' * 256
        self.assertIn('name may be at most 255 chars', str(cm.exception))

    def testLpxStatus(self):
        lp = LPX()
        self.assertEqual('undef', lp.status_s)

    # TODO: how to write a test where the ray returned is a row?
    def testLpxRay(self):
        lp = LPX()
        self.assertTrue(lp.ray is None)
        lp.cols.add(2)
        x1, x2 = lp.cols[0:2]
        lp.obj[:] = [-1.0, 1.0]
        lp.rows.add(2)
        r1, r2 = lp.rows[0:2]
        r1.matrix = [1.0, -1.0]
        r1.bounds = (-3, None)
        r2.matrix = [-1.0, 2.0]
        r2.bounds = (-2.0, None)
        x1.bounds = None
        x2.bounds = None
        lp.simplex()

        self.assertEqual(lp.status, 'unbnd')
        self.assertTrue(lp.ray.iscol)


class ObjectiveTests(unittest.TestCase):

    def testObjectiveShift(self):
        lp = LPX()
        with self.assertRaises(TypeError) as cm:
            del(lp.obj.shift)
        self.assertIn('deletion not supported', str(cm.exception))

    def testObjectiveAssignment(self):
        lp = LPX()
        with self.assertRaises(TypeError) as cm:
            del(lp.obj[0])
        self.assertIn(
            "objective function doesn't support item deletion",
            str(cm.exception)
        )

    def testObjectiveName(self):
        lp = LPX()
        with self.assertRaises(ValueError) as cm:
            lp.obj.name = 'a' * 256
        self.assertIn('name may be at most 255 chars', str(cm.exception))

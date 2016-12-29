"""Tests for basic functionality"""

import sys
import unittest

from glpk import LPX


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

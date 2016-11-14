"""Tests for reading and writing problems and solutions."""

import tempfile
import unittest

from glpk import LPX


class WriteTests(unittest.TestCase):
    """A simple suite of tests for this problem.

    max (x+y) subject to
    0.5*x + y <= 1
    0 <= x <= 1
    0 <= y <= 1

    This should have optimal solution x=1, y=0.5."""
    def setUp(self):
        # create a new temp file for reading/writing to
        self.f = tempfile.NamedTemporaryFile()
        lp = self.lp = LPX()
        lp.rows.add(1)
        lp.cols.add(2)
        lp.cols[0].name = 'x'
        lp.cols[1].name = 'y'
        for c in lp.cols:
            c.bounds = 0, 1
        lp.obj[:] = [1, 1]
        lp.obj.maximize = True
        lp.rows[0].matrix = [0.5, 1.0]
        lp.rows[0].bounds = None, 1

    def tearDown(self):
        # close (delete) the temp file
        self.f.close()

    def testMps(self):
        """Test reading/writing MPS format."""
        self.lp.write(mps=self.f.name)
        lp = LPX(mps=self.f.name)
        self.assertEqual(len(lp.rows), 1)
        self.assertEqual(len(lp.cols), 2)
        self.assertEqual(lp.cols[0].name, 'x')
        self.assertEqual(lp.cols[1].name, 'y')

        # try to read a non-existing file
        with self.assertRaises(RuntimeError) as cm:
            lp = LPX(mps='not a real file')
        self.assertIn('MPS reader failed', str(cm.exception))

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            lp.write(mps='not/a/real/file')
        self.assertIn(
            "writer for 'mps' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testFreeMps(self):
        """Test reading/writing free MPS format."""
        self.lp.write(freemps=self.f.name)
        lp = LPX(freemps=self.f.name)
        self.assertEqual(len(lp.rows), 1)
        self.assertEqual(len(lp.cols), 2)
        self.assertEqual(lp.cols[0].name, 'x')
        self.assertEqual(lp.cols[1].name, 'y')

        # try to read a non-existing file
        with self.assertRaises(RuntimeError) as cm:
            lp = LPX(freemps='not a real file')
        self.assertIn('Free MPS reader failed', str(cm.exception))

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            lp.write(freemps='not/a/real/file')
        self.assertIn(
            "writer for 'freemps' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testCpxLp(self):
        """Test reading/writing CPLEX LP format."""
        self.lp.write(cpxlp=self.f.name)
        lp = LPX(cpxlp=self.f.name)
        self.assertEqual(len(lp.rows), 1)
        self.assertEqual(len(lp.cols), 2)
        self.assertEqual(lp.cols[0].name, 'x')
        self.assertEqual(lp.cols[1].name, 'y')

        # try to read a non-existing file
        with self.assertRaises(RuntimeError) as cm:
            lp = LPX(cpxlp='not a real file')
        self.assertIn('CPLEX LP reader failed', str(cm.exception))

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            lp.write(cpxlp='not/a/real/file')
        self.assertIn(
            "writer for 'cpxlp' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testGlp(self):
        """Test reading/writing GNU LP format."""
        self.lp.write(glp=self.f.name)
        lp = LPX(glp=self.f.name)
        self.assertEqual(len(lp.rows), 1)
        self.assertEqual(len(lp.cols), 2)
        self.assertEqual(lp.cols[0].name, 'x')
        self.assertEqual(lp.cols[1].name, 'y')

        # try to read a non-existing file
        with self.assertRaises(RuntimeError) as cm:
            lp = LPX(glp='not a real file')
        self.assertIn('GLPK LP/MIP reader failed', str(cm.exception))

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            lp.write(glp='not/a/real/file')
        self.assertIn(
            "writer for 'glp' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testGmp(self):
        """Test reading/writing GNU MathProg format."""
        # there is no write keyword for 'gmp'; write the file manually
        self.f.write('''
        var x, >= 0, <= 1;
        var y, >= 0, <= 1;
        maximize
        value: x + y;
        subject to
        row: .5*x + y <= 1;
        end;
        '''.encode('utf-8'))
        self.f.flush()
        lp = LPX(gmp=self.f.name)
        # TODO: why is this two?
        self.assertEqual(len(lp.rows), 2)
        self.assertEqual(len(lp.cols), 2)
        self.assertEqual(lp.cols[0].name, 'x')
        self.assertEqual(lp.cols[1].name, 'y')

        # try to read a non-existing file
        with self.assertRaises(RuntimeError) as cm:
            lp = LPX(gmp='not a real file')
        self.assertIn('GMP model reader failed', str(cm.exception))

        # try to write to a non-existing path
        with self.assertRaises(ValueError) as cm:
            lp = LPX(gmp=())
        self.assertIn('model tuple must have 1<=length<=3', str(cm.exception))

        with self.assertRaises(TypeError) as cm:
            lp = LPX(gmp={})
        self.assertIn('model arg must be string or tuple', str(cm.exception))

    def testWriteSol(self):
        """Test writing basic solution."""
        self.lp.simplex()
        self.lp.write(sol=self.f.name)

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            self.lp.write(sol='not/a/real/file')
        self.assertIn(
            "writer for 'sol' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testWriteSensBnds(self):
        """Test writing bounds sensitivity."""
        self.lp.simplex()
        self.lp.write(sens_bnds=self.f.name)

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            self.lp.write(sens_bnds='not/a/real/file')
        self.assertIn(
            "writer for 'sens_bnds' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testWriteIps(self):
        """Test writing interior-point solution."""
        self.lp.interior()
        self.lp.write(ips=self.f.name)

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            self.lp.write(ips='not/a/real/file')
        self.assertIn(
            "writer for 'ips' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

    def testWriteMip(self):
        """Test writing MIP solution."""
        self.lp.simplex()
        self.lp.integer()
        self.lp.write(mip=self.f.name)

        # try to write to a non-existing path
        with self.assertRaises(RuntimeError) as cm:
            self.lp.write(mip='not/a/real/file')
        self.assertIn(
            "writer for 'mip' failed to write to 'not/a/real/file'",
            str(cm.exception)
        )

"""Tests for setting up the LP constraint matrix."""


import glpk
import random
import unittest


class MatrixCoefficientTestCase(unittest.TestCase):
    """Tests setting and reading the coefficient matrix."""
    def setUp(self):
        self.lp = glpk.LPX()
        self.lp.rows.add(3)
        self.lp.cols.add(4)
        for row in self.lp.rows:
            row.name = 'r%d' % row.index
        for col in self.lp.cols:
            col.name = 'c%d' % col.index
        self.matrix = [
            (0, 0, 2), (0, 1, 3), (0, 2, -2), (1, 1, 3.5), (1, 2, -5),
            (1, 3, 10), (2, 0, 7), (2, 1, 8), (2, 2, -3), (2, 3, 900)
        ]

    # Convenience methods for running comparison and extracting components.
    def rowVector(self, rnum, mat=None):
        if mat is None:
            mat = self.matrix
        return [(c, val) for r, c, val in mat if r == rnum]

    def colVector(self, cnum, mat=None):
        if mat is None:
            mat = self.matrix
        return [(r, val) for r, c, val in mat if c == cnum]

    def checkMatrix(self, mat=None):
        """Typical final equality check for our test functions."""
        # Compare matrix all at once, and also piecemeal by row and
        # column individually.
        if mat is None:
            mat = self.matrix
        self.assertEqual(self.lp.matrix, mat)
        for row in self.lp.rows:
            self.assertEqual(row.matrix, self.rowVector(row.index, mat))
        for col in self.lp.cols:
            self.assertEqual(col.matrix, self.colVector(col.index, mat))

    # This is the matrix used throughout this example:
    # [ 2,   3, -2,   0 ]
    # [ 0, 3.5, -5,  10 ]
    # [ 7,   8, -3, 900 ]

    def testInitialMatrix(self):
        """Test that the coefficient matrix is initially empty."""
        self.assertEqual(len(self.lp.matrix), 0)
        self.checkMatrix([])

    def testMatrixSetWithExplicitIndices(self):
        """Test setting the matrix with only explicit indices."""
        self.lp.matrix = self.matrix
        self.checkMatrix()

    def testMatrixSetWithExplicitStringIndices(self):
        """Test setting the matrix with explicit string indices."""
        matrix = [('r%d' % r, 'c%d' % c, v) for r, c, v in self.matrix]
        self.lp.matrix = matrix
        self.checkMatrix()

    def testMatrixSetWithExplicitIndicesOutOfOrder(self):
        """Test setting matrix with indices out of order."""
        rgen = random.Random(1)
        matrix = self.matrix[:]  # Shuffle only a copy.
        rgen.shuffle(matrix)
        self.lp.matrix = matrix
        self.checkMatrix()

    def testMatrixSetWithImplicitIndices(self):
        """Test setting the matrix with only implicit indices."""
        imatrix = [0.0 for r in range(len(self.lp.rows))
                   for c in range(len(self.lp.cols))]
        for r, c, v in self.matrix:
            imatrix[r*len(self.lp.cols) + c] = float(v)
        self.lp.matrix = imatrix
        self.checkMatrix()

    def testMatrixUnsetWithEmptyAssignment(self):
        """Test removing the matrix by assigning to []."""
        self.lp.matrix = self.matrix
        self.checkMatrix()
        self.lp.matrix = []
        self.checkMatrix([])

    def testMatrixUnsetWithDelete(self):
        """Test removing the matrix by a del upon it."""
        self.lp.matrix = self.matrix
        self.checkMatrix()
        del self.lp.matrix
        self.checkMatrix([])

    def testSetRowVectorsWithExplicitIndices(self):
        """Test setting the matrix row by row."""
        for row in self.lp.rows:
            row.matrix = self.rowVector(row.index)
        self.checkMatrix()

    def testSetRowVectorsWithExplicitStringIndices(self):
        """Test setting the matrix row by row with string column indices."""
        for row in self.lp.rows:
            row.matrix = [('c%d' % c, v) for c, v in self.rowVector(row.index)]
        self.checkMatrix()

    def testSetRowVectorsWithImplicitIndices(self):
        """Test setting the matrix row by row with no indices."""
        for row in self.lp.rows:
            vec = [0.0 for i in range(len(self.lp.cols))]
            for c, v in self.rowVector(row.index):
                vec[c] = v
            row.matrix = vec
        self.checkMatrix()

    def testSetColumnVectorsWithExplicitIndices(self):
        """Test setting the matrix column by column."""
        for col in self.lp.cols:
            col.matrix = self.colVector(col.index)
        self.checkMatrix()

    def testSetColumnVectorsWithExplicitStringIndices(self):
        """Test setting the matrix column by column with string row indices."""
        for col in self.lp.cols:
            col.matrix = [('r%d' % r, v) for r, v in self.colVector(col.index)]
        self.checkMatrix()

    def testSetColumnVectorsWithImplicitIndices(self):
        """Test setting the matrix column by column with no indices."""
        for col in self.lp.cols:
            vec = [0.0 for i in range(len(self.lp.rows))]
            for r, v in self.colVector(col.index):
                vec[r] = v
            col.matrix = vec
        self.checkMatrix()

    def testUnsetRowVectors(self):
        """Test unsetting rows with empty assignments and deletion."""
        mat = self.matrix
        # Initial assignment OK.
        self.lp.matrix = mat
        self.checkMatrix()
        # Try unsetting a row by assigning its matrix to [].
        self.lp.rows[1].matrix = []
        mat = [(r, c, v) for r, c, v in mat if r != 1]
        self.checkMatrix(mat)
        # Try unsetting a row by deleting the matrix.
        del self.lp.rows[2].matrix
        mat = [(r, c, v) for r, c, v in mat if r != 2]
        self.checkMatrix(mat)
        # Try unsetting a row with assignment to 0.
        self.lp.rows[0].matrix = [0, 0, 0, 0]
        mat = [(r, c, v) for r, c, v in mat if r != 0]
        self.checkMatrix(mat)

    def testUnsetColumnVectors(self):
        """Test unsetting rows with empty assignments and deletion."""
        mat = self.matrix
        # Initial assignment OK.
        self.lp.matrix = mat
        self.checkMatrix()
        # Try unsetting a row by assigning its matrix to [].
        self.lp.cols[1].matrix = []
        mat = [(r, c, v) for r, c, v in mat if c != 1]
        self.checkMatrix(mat)
        # Try unsetting a row by deleting the matrix.
        del self.lp.cols[2].matrix
        mat = [(r, c, v) for r, c, v in mat if c != 2]
        self.checkMatrix(mat)
        # Try unsetting a column with assignment to 0.
        self.lp.cols[0].matrix = [0, 0, 0]
        mat = [(r, c, v) for r, c, v in mat if c != 0]
        self.checkMatrix(mat)

    def testResetMatrix(self):
        """Tests setting the matrix, then setting it to something new."""
        mat = self.matrix
        # Initial assignment OK.
        self.lp.matrix = mat
        self.checkMatrix()
        # Shift the rows down one.
        mat = [((r+1) % len(self.lp.rows), c, v) for r, c, v in mat]
        self.lp.matrix = mat
        mat.sort()
        self.checkMatrix(mat)

    def testMatrixWithVectorDeletion(self):
        """Tests that the matrix is sane when rows/columns are deleted."""
        mat = self.matrix
        # Initial assignment OK.
        self.lp.matrix = mat
        self.checkMatrix()
        # Delete some row.
        del self.lp.rows[1]
        mat = [(r, c, v) for r, c, v in mat if r < 1] + [
            (r-1, c, v) for r, c, v in mat if r > 1]
        mat.sort()
        self.checkMatrix(mat)
        # Delete some column.
        del self.lp.cols[2]
        mat = [(r, c, v) for r, c, v in mat if c < 2] + [
            (r, c-1, v) for r, c, v in mat if c > 2]
        mat.sort()
        self.checkMatrix(mat)

    def testNumberNonZeroAttribute(self):
        """Tests nnz in the LP and in vectors."""
        # Run the initial test.
        self.assertEqual(self.lp.nnz, 0)
        for row in self.lp.rows:
            self.assertEqual(row.nnz, 0)
        for col in self.lp.cols:
            self.assertEqual(col.nnz, 0)
        # Set the matrix.
        mat = self.matrix
        self.lp.matrix = mat
        self.assertEqual(self.lp.nnz, len(mat))
        self.assertEqual([r.nnz for r in self.lp.rows], [3, 3, 4])
        self.assertEqual([c.nnz for c in self.lp.cols], [2, 3, 3, 2])
        # Clear a row.
        del self.lp.rows[1].matrix
        self.assertEqual(self.lp.nnz, len(mat)-3)
        self.assertEqual([r.nnz for r in self.lp.rows], [3, 0, 4])
        self.assertEqual([c.nnz for c in self.lp.cols], [2, 2, 2, 1])
        # Delete a column outright.
        del self.lp.cols[-1]
        self.assertEqual(self.lp.nnz, len(mat)-4)
        self.assertEqual([r.nnz for r in self.lp.rows], [3, 0, 3])
        self.assertEqual([c.nnz for c in self.lp.cols], [2, 2, 2])
        # Clear the matrix.
        del self.lp.matrix
        self.assertEqual(self.lp.nnz, 0)
        self.assertEqual([r.nnz for r in self.lp.rows], [0, 0, 0])
        self.assertEqual([c.nnz for c in self.lp.cols], [0, 0, 0])

    def testMatrixSetBadValueTypesWithExplicitIndexing(self):
        """Test setting the explicitly-indexed matrix with bad value types."""
        with self.assertRaises(TypeError):
            self.lp.matrix = [(0, 1, 2), (0, 2, "hello"), (1, 3, [4])]

    def testMatrixSetBadValueTypesWithImplicitIndexing(self):
        """Test setting the implicitly-indexed matrix with bad value types."""
        with self.assertRaises(TypeError):
            self.lp.matrix = ["hello", [4]]

    def testMatrixSetVectorBadValueTypesWithExplicitIndexing(self):
        """Test setting explicitly-indexed vectors with bad value types."""
        with self.assertRaises(TypeError):
            self.lp.rows[1].matrix = [(0, 5), (1, int), (2, "hi!")]

    def testMatrixSetVectorBadValueTypesWithImplicitIndexing(self):
        """Test setting implicitly-indexed vectors with bad value types."""
        with self.assertRaises(TypeError):
            self.lp.rows[1].matrix = [5, int, "hi!"]
        with self.assertRaises(TypeError):
            self.lp.cols[1].matrix = [5, int, "hi!"]

    def testMatrixSetBadIndexTypes(self):
        """Test setting a matrix with bad index types."""
        with self.assertRaises(TypeError):
            self.lp.matrix = [(0, 1, 2), (0, {"hi": 2}, 3.1415), (1, 3, 4)]
        with self.assertRaises(TypeError):
            self.lp.matrix = [(0, 1, 2), (0, 2, 3.1415), (1, [3], 4)]

    def testVectorSetBadIndexTypes(self):
        """Test setting vectors with bad index types."""
        with self.assertRaises(TypeError):
            self.lp.rows[1].matrix = [(1, 2), ({"hi": 2}, 3.1415), (3, 4)]
        with self.assertRaises(TypeError):
            self.lp.cols[2].matrix = [(1, 2), (2, 3.1415), ([3], 4)]

    def testMatrixSetWithOutOfRangeExplicitIndexing(self):
        """Test setting the matrix with explicit indices out of range."""
        with self.assertRaises(IndexError):
            self.lp.matrix = [(0, 1, 2), (6, 6, 2)]
        with self.assertRaises(IndexError):
            self.lp.matrix = [(0, -5, 2), (0, 2, 3.1415), (1, 3, 4)]

    def testMatrixSetWithOutOfRangeImplicitIndexing(self):
        """Test setting the matrix with implicit indices out of range."""
        with self.assertRaises(IndexError):
            self.lp.matrix = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
        with self.assertRaises(IndexError):
            self.lp.matrix = [(2, 3, 4), 5]

    def testVectorSetWithOutOfRangeExplicitIndexing(self):
        """Test setting vectors with explicit indices out of range."""
        with self.assertRaises(IndexError):
            self.lp.rows[1].matrix = [(1, 2), (3, 4), (4, 8)]
        with self.assertRaises(IndexError):
            self.lp.cols[2].matrix = [(1, 2), (2, 3.1415), (3, 4)]

    def testVectorSetWithOutOfRangeImplicitIndexing(self):
        """Test setting vectors with implicit indices out of range."""
        with self.assertRaises(IndexError):
            self.lp.rows[1].matrix = [1, 2, 3, 4, 5]
        with self.assertRaises(IndexError):
            self.lp.cols[2].matrix = [(2, 3.1415), 1]

    def testMatrixSetWithDuplicateIndices(self):
        """Test setting the matrix with duplicate indices."""
        with self.assertRaises(ValueError):
            self.lp.matrix = [(0, 1, 2), (0, 1, 3)]
        with self.assertRaises(ValueError):
            self.lp.matrix = [(2, 0, 2), (1, 3, 3.1415), 4]

    def testVectorSetWithDuplicateIndices(self):
        """Test setting vector matrix entries with duplicate indices."""
        with self.assertRaises(ValueError):
            self.lp.rows[1].matrix = [(1, 2), (0, 4), 8]
        with self.assertRaises(ValueError):
            self.lp.cols[2].matrix = [(1, 2), (2, 3.1415), (1, 4)]


class MatrixScalingTestCase(unittest.TestCase):
    """Tests the scale and unscale functions."""
    def setUp(self):
        self.lp = glpk.LPX()
        self.lp.rows.add(100)
        self.lp.cols.add(100)
        rgen = random.Random(0)
        newmatrix = []
        # This should be pretty poorly conditioned.
        for row in self.lp.rows:
            scale = rgen.uniform(0, 1000)
            newmatrix.extend(rgen.uniform(-scale, scale) for c in self.lp.cols)
        self.lp.matrix = newmatrix

    def testScaling(self):
        """Tests default scaling with the LPX.scale method."""
        self.lp.scale()

    @unittest.skipUnless(
        glpk.env.version > (4, 31),
        'Parameterized scaling is only available as of GLPK 4.31.'
    )
    def testFlaggedScaling(self):
        """Tests scaling with flags."""
        self.lp.scale(glpk.LPX.SF_EQ | glpk.LPX.SF_2N)

    def testUnscaling(self):
        """Tests the LPX.unscale method."""
        self.lp.scale()
        self.lp.unscale()
        self.assertEqual(set([1.0]), set(r.scale for r in self.lp.rows) |
                         set(c.scale for c in self.lp.cols))

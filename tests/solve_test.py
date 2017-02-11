"""Tests for the solver itself."""

from glpk import env, LPX
import sys
import unittest
from itertools import cycle


class SimpleSolverTest(unittest.TestCase):
    """A simple suite of tests for this problem.

    max (x+y) subject to
    0.5*x + y <= 1
    0 <= x <= 1
    0 <= y <= 1

    This should have optimal solution x=1, y=0.5."""
    def setUp(self):
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

    def testSimplex(self):
        """Tests solving the simple problem with simplex."""
        self.lp.simplex()
        self.assertAlmostEqual(self.lp.cols['x'].value, 1.0)
        self.assertAlmostEqual(self.lp.cols['y'].value, 0.5)
        self.assertAlmostEqual(self.lp.obj.value_s, 1.5)

    def testExact(self):
        """Tests solving the simple problem with exact."""
        self.lp.exact()
        self.assertEqual(self.lp.cols['x'].value, 1.0)
        self.assertEqual(self.lp.cols['y'].value, 0.5)

    def testInterior(self):
        """Tests solving the simple problem with interior."""
        self.lp.interior()
        self.assertAlmostEqual(self.lp.cols['x'].value, 1.0)
        self.assertAlmostEqual(self.lp.cols['y'].value, 0.5)

    def testSimplexKKT(self):
        """Tests the KKT check with solution from simplex solver."""
        # Solve test LP using the simplex method,
        # check KKT conditions on optimal solution
        self.lp.simplex()
        kkt = self.lp.kkt()

        # The test LP has an optimal solution.
        # The KKT conditions should be (approximately) satisfied.
        self.assertAlmostEqual(kkt.pe_ae_max, 0.0)
        self.assertAlmostEqual(kkt.pe_re_max, 0.0)
        self.assertAlmostEqual(kkt.pb_ae_max, 0.0)
        self.assertAlmostEqual(kkt.pb_re_max, 0.0)
        self.assertAlmostEqual(kkt.de_ae_max, 0.0)
        self.assertAlmostEqual(kkt.de_re_max, 0.0)
        self.assertAlmostEqual(kkt.db_ae_max, 0.0)
        self.assertAlmostEqual(kkt.db_re_max, 0.0)

        # Are the row/column/variable indices valid?
        # In the test LP, there are:
        # - 1 row
        # - 2 columns
        # - 3 variables (rows + columns)
        # If the error reported is exactly 0, then glp_check_kkt
        # returns an index of 0.

        # Primal equality constraints (one for each row)
        self.assertTrue(kkt.pe_ae_row in [0, 1])
        self.assertTrue(kkt.pe_re_row in [0, 1])

        # Primal bound constraints (one for each variable)
        # There are one row and two columns in the test LP.
        self.assertTrue(kkt.pb_ae_ind in [0, 1, 2, 3])
        self.assertTrue(kkt.pb_re_ind in [0, 1, 2, 3])

        # Dual equality constraints (one for each column)
        self.assertTrue(kkt.de_ae_row in [0, 1, 2])
        self.assertTrue(kkt.de_re_row in [0, 1, 2])

        # Dual bound constraints (one for each variable)
        self.assertTrue(kkt.db_ae_ind in [0, 1, 2, 3])
        self.assertTrue(kkt.db_re_ind in [0, 1, 2, 3])

        # Are the KKT "quality" values valid?
        self.assertTrue(kkt.pe_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.pb_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.de_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.db_quality in ['H', 'M', 'L', '?'])

    def testExactKKT(self):
        """Tests the KKT check with solution from exact solver."""
        # Solve test LP using the simplex method,
        # check KKT conditions on optimal solution
        self.lp.exact()
        kkt = self.lp.kkt()

        # The test LP has an optimal solution.
        # The KKT conditions should be (approximately) satisfied.
        self.assertAlmostEqual(kkt.pe_ae_max, 0.0)
        self.assertAlmostEqual(kkt.pe_re_max, 0.0)
        self.assertAlmostEqual(kkt.pb_ae_max, 0.0)
        self.assertAlmostEqual(kkt.pb_re_max, 0.0)
        self.assertAlmostEqual(kkt.de_ae_max, 0.0)
        self.assertAlmostEqual(kkt.de_re_max, 0.0)
        self.assertAlmostEqual(kkt.db_ae_max, 0.0)
        self.assertAlmostEqual(kkt.db_re_max, 0.0)

        # Are the row/column/variable indices valid?
        # In the test LP, there are:
        # - 1 row
        # - 2 columns
        # - 3 variables (rows + columns)
        # If the error reported is exactly 0, then glp_check_kkt
        # returns an index of 0.

        # Primal equality constraints (one for each row)
        self.assertTrue(kkt.pe_ae_row in [0, 1])
        self.assertTrue(kkt.pe_re_row in [0, 1])

        # Primal bound constraints (one for each variable)
        # There are one row and two columns in the test LP.
        self.assertTrue(kkt.pb_ae_ind in [0, 1, 2, 3])
        self.assertTrue(kkt.pb_re_ind in [0, 1, 2, 3])

        # Dual equality constraints (one for each column)
        self.assertTrue(kkt.de_ae_row in [0, 1, 2])
        self.assertTrue(kkt.de_re_row in [0, 1, 2])

        # Dual bound constraints (one for each variable)
        self.assertTrue(kkt.db_ae_ind in [0, 1, 2, 3])
        self.assertTrue(kkt.db_re_ind in [0, 1, 2, 3])

        # Are the KKT "quality" values valid?
        self.assertTrue(kkt.pe_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.pb_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.de_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.db_quality in ['H', 'M', 'L', '?'])

    def testAdvBasis(self):
        # adding for coverage
        # TODO: how to test this?
        self.lp.adv_basis()
        self.assertTrue(self.lp.simplex() is None)
        self.assertEqual('opt', self.lp.status)

    def testCpxBasis(self):
        # adding for coverage
        # TODO: how to test this?
        self.lp.cpx_basis()
        self.assertTrue(self.lp.simplex() is None)
        self.assertEqual('opt', self.lp.status)


class SimpleIntegerSolverTest(unittest.TestCase):
    """A simple suite of tests for this problem.

    max (2*x+y) subject to
    0.5*x + y <= 1
    0 <= x <= 1
    0 <= y <= 1
    x, y integer

    This should have optimal solution x=1, y=0."""
    def setUp(self):
        lp = self.lp = LPX()
        lp.rows.add(1)
        lp.cols.add(2)
        lp.cols[0].name = 'x'
        lp.cols[1].name = 'y'
        for c in lp.cols:
            c.bounds = 0, 1
            c.kind = int
        lp.obj[:] = [2, 1]
        lp.obj.maximize = True
        lp.rows[0].matrix = [0.5, 1.0]
        lp.rows[0].bounds = None, 1

    def testInteger(self):
        """Tests solving the simple problem with integer."""
        class Callback:
            def default(self, tree):
                pass
        self.lp.integer(presolve=True, callback=Callback())
        self.assertAlmostEqual(self.lp.cols['x'].value, 1.0)
        self.assertAlmostEqual(self.lp.cols['y'].value, 0.0)
        self.assertAlmostEqual(self.lp.obj.value, 2.0)

    def testIntegerKKT(self):
        """Tests the KKT check with solution from integer solver."""
        # Solve test LP using the simplex method,
        # check KKT conditions on optimal solution
        self.lp.integer(presolve=True)
        kkt = self.lp.kktint()

        # The test IP has an optimal solution.
        # The primal KKT conditions should be (approximately) satisfied.
        self.assertAlmostEqual(kkt.pe_ae_max, 0.0)
        self.assertAlmostEqual(kkt.pe_re_max, 0.0)
        self.assertAlmostEqual(kkt.pb_ae_max, 0.0)
        self.assertAlmostEqual(kkt.pb_re_max, 0.0)

        # Are the row/column/variable indices valid?
        # In the test IP, there are:
        # - 1 row
        # - 2 columns
        # - 3 variables (rows + columns)
        # If the error reported is exactly 0, then glp_check_kkt
        # returns an index of 0.

        # Primal equality constraints (one for each row)
        self.assertTrue(kkt.pe_ae_row in [0, 1])
        self.assertTrue(kkt.pe_re_row in [0, 1])

        # Primal bound constraints (one for each variable)
        # There are one row and two columns in the test LP.
        self.assertTrue(kkt.pb_ae_ind in [0, 1, 2, 3])
        self.assertTrue(kkt.pb_re_ind in [0, 1, 2, 3])

        # Are the KKT "quality" values valid?
        self.assertTrue(kkt.pe_quality in ['H', 'M', 'L', '?'])
        self.assertTrue(kkt.pb_quality in ['H', 'M', 'L', '?'])

    def testIntOpt(self):
        """Test solving a simple integer program w/ intopt."""
        # FIXME: documentation claims that this method does not require an
        # existing LP relaxation
        self.lp.simplex()
        self.lp.intopt()
        self.assertEqual(self.lp.status, 'opt')
        self.assertAlmostEqual(self.lp.cols['x'].value, 1.0)
        self.assertAlmostEqual(self.lp.cols['y'].value, 0.0)


class TwoDimensionalTest(unittest.TestCase):
    def setUp(self):
        self.lp = LPX()

    def testEvolvingConstraintsSimplex(self):
        """Test repeatedly simplex solving an LP with evolving constraints."""
        lp = self.lp
        # Set up the rules of the problem.
        lp.cols.add(2)
        lp.obj[0, 1] = 1
        # Try very simple rules.
        x1, x2 = lp.cols[0], lp.cols[1]  # For convenience...
        x1.name, x2.name = 'x1', 'x2'
        x1.bounds = None, 1
        x2.bounds = None, 2
        lp.obj.maximize = True
        self.assertEqual(None, lp.simplex())
        self.assertEqual('opt', lp.status)
        self.assertAlmostEqual(lp.obj.value, 3)
        self.assertAlmostEqual(x1.primal, 1)
        self.assertAlmostEqual(x2.primal, 2)
        # Now try pushing it into unbounded territory.
        lp.obj[0] = -1
        self.assertEqual(None, lp.simplex())  # No error?
        self.assertEqual('unbnd', lp.status)
        # Redefine the bounds so it is bounded again.
        x1.bounds = -1, None
        self.assertEqual(None, lp.simplex())
        self.assertEqual('opt', lp.status)

        self.assertAlmostEqual(lp.obj.value, 3)
        self.assertAlmostEqual(x1.primal, -1)
        self.assertAlmostEqual(x2.primal, 2)
        # Now add in a row constraint forcing it to a new point, (1/2, 2).
        lp.rows.add(1)
        lp.rows[0].matrix = [-2, 1]
        lp.rows[0].bounds = None, 1
        self.assertEqual(None, lp.simplex())
        self.assertEqual('opt', lp.status)
        self.assertAlmostEqual(lp.obj.value, 1.5)
        self.assertAlmostEqual(x1.primal, .5)
        self.assertAlmostEqual(x2.primal, 2)
        # Now add in another forcing it to point (1/4, 3/2).
        lp.rows.add(1)
        lp.rows[-1].matrix = [-2, -1]
        lp.rows[-1].bounds = -2, None

        self.assertEqual(None, lp.simplex())
        self.assertEqual('opt', lp.status)

        self.assertAlmostEqual(lp.obj.value, 1.25)
        self.assertAlmostEqual(x1.primal, .25)
        self.assertAlmostEqual(x2.primal, 1.5)
        # Now go for the gusto.  Change column constraint, force infeasibility.
        x2.bounds = 2, None  # Instead of x2<=2, must now be >=2!  Tee hee.
        self.assertEqual(None, lp.simplex())
        self.assertEqual('nofeas', lp.status)
        # By removing the first row constraint, we allow opt point (-1, 1).
        del lp.rows[0]
        lp.std_basis()

        self.assertEqual(None, lp.simplex())
        self.assertEqual('opt', lp.status)

        self.assertAlmostEqual(lp.obj.value, 5)
        self.assertAlmostEqual(x1.primal, -1)
        self.assertAlmostEqual(x2.primal, 4)

    def testEvolvingConstraintsInterior(self):
        """Similar, but for the interior point method."""
        lp = self.lp
        # Set up the rules of the problem.
        lp.cols.add(2)
        lp.obj[0, 1] = 1
        # Try very simple rules.
        x1, x2 = lp.cols[0], lp.cols[1]  # For convenience...
        x1.name, x2.name = 'x1', 'x2'
        x1.bounds = None, 1
        x2.bounds = None, 2
        lp.obj.maximize = True
        # This should fail, since interior point methods need some
        # rows, and some columns.
        self.assertEqual('fail', lp.interior())
        # Now try pushing it into unbounded territory.
        lp.obj[0] = -1
        # Redefine the bounds so it is bounded again.
        x1.bounds = -1, None
        # Now add in a row constraint forcing it to a new point, (1/2, 2).
        lp.rows.add(1)
        lp.rows[0].matrix = [-2, 1]
        lp.rows[0].bounds = None, 1
        self.assertEqual(None, lp.interior())
        self.assertEqual('opt', lp.status)
        self.assertAlmostEqual(lp.obj.value, 1.5)
        self.assertAlmostEqual(x1.primal, .5)
        self.assertAlmostEqual(x2.primal, 2)
        # Now add in another forcing it to point (1/4, 3/2).
        lp.rows.add(1)
        lp.rows[-1].matrix = [-2, -1]
        lp.rows[-1].bounds = -2, None

        self.assertEqual(None, lp.interior())
        self.assertEqual('opt', lp.status)

        self.assertAlmostEqual(lp.obj.value, 1.25)
        self.assertAlmostEqual(x1.primal, .25)
        self.assertAlmostEqual(x2.primal, 1.5)
        # Now go for the gusto.  Change column constraint, force infeasibility.
        x2.bounds = 3, None  # Instead of x2<=2, must now be >=3!  Tee hee.
        self.assertEqual(None, lp.interior())
        self.assertEqual('nofeas', lp.status)
        # By removing the first row constraint, we allow opt point (-1, 4).
        del lp.rows[0]
        lp.std_basis()

        self.assertEqual(None, lp.interior())
        self.assertEqual('opt', lp.status)

        self.assertAlmostEqual(lp.obj.value, 5)
        self.assertAlmostEqual(x1.primal, -1)
        self.assertAlmostEqual(x2.primal, 4)


class SatisfiabilityMIPTest(unittest.TestCase):
    @classmethod
    def solve_sat(self, expression):
        """Attempts to satisfy a formula of conjunction of disjunctions.

        If there are n variables in the expression, this will return a
        list of length n, all elements booleans.  The truth of element i-1
        corresponds to the truth of variable i.

        If no satisfying assignment could be found, None is returned."""
        if len(expression) == 0:
            return []
        numvars = max(max(abs(v) for v in clause) for clause in expression)
        lp = LPX()
        lp.cols.add(2*numvars)
        for col in lp.cols:
            col.bounds = 0.0, 1.0

        def lit2col(lit):
            if lit > 0:
                return 2*lit-2
            return 2*(-lit)-1

        for i in range(1, numvars+1):
            lp.cols[lit2col(i)].name = 'x_%d' % i
            lp.cols[lit2col(-i)].name = '!x_%d' % i
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
            lp.rows[-1].bounds = 1.0
        for clause in expression:
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
            lp.rows[-1].bounds = 1, None
        retval = lp.simplex()
        if retval is not None:
            return None
        if lp.status != 'opt':
            return None
        for col in lp.cols:
            col.kind = int
        retval = lp.integer()
        if retval is not None:
            return None
        if lp.status != 'opt':
            return None
        return [col.value > 0.99 for col in lp.cols[::2]]

    @classmethod
    def verify(self, expression, assignment):
        """Get the truth of an expression given a variable truth assignment.

        This will return true only if this is a satisfying assignment."""
        # Each clause must be true.
        for clause in expression:
            # For a disjunctive clause to be true, at least one of its
            # literals must be true.
            lits_true = 0
            for lit in clause:
                if (lit > 0) == (assignment[abs(lit)-1]):
                    lits_true += 1
            if lits_true == 0:
                return False
        return True

    def testSolvableSat(self):
        """Test that a solvable satisfiability problem can be solved."""
        exp = [(3, -2, 5), (1, 2, 5), (1, 3, -4), (-1, -5, -3), (-4, 2, 1),
               (5, 2, -1), (5, 2, 1), (-4, -1, -5), (4, 5, -1), (3, 1, -2),
               (1, 5, -3), (-5, -3, -1), (-4, -5, -3), (-3, -5, -2),
               (4, -5, 3), (1, -2, -5), (1, 4, -3), (4, -1, -3),
               (-5, -1, 3), (-2, -4, -5)]
        solution = self.solve_sat(exp)
        # First assert that this is a solution.
        self.assertNotEqual(solution, None)
        self.assertTrue(self.verify(exp, solution))

    def testInsolvableSat(self):
        """Test that an unsolvable satisfiability problem can't be solved."""
        exp = [(1, 2), (1, -2), (-1, 2), (-1, -2)]
        solution = self.solve_sat(exp)
        self.assertEqual(solution, None)


@unittest.skipIf(
    env.version < (4, 20),
    "callbacks did't exist prior to 4.20"
)
class MIPCallbackTest(unittest.TestCase):
    # This CNF expression is complicated enough to the point where
    # when run through our SAT solver, the resulting search tree will
    # have a fair number of calls, and the callback shall be called a
    # number of times.  Incidentally, it does have valid assignment
    # FFFTTTTFTFTFTTFFFFFTTTFFTTTTTT.
    expression = [
        (-5, -26, -23), (-20, 24, -3), (23, -1, 14), (28, -1, -17),
        (-13, -1, -7), (-7, 14, 9), (20, -6, -30), (22, 29, -13),
        (18, 27, -26), (-8, 24, 13), (21, 12, 14), (-12, 15, 1), (18, 12, 6),
        (17, 26, 7), (14, -9, 17), (25, 27, 23), (13, -2, 27), (15, 11, -17),
        (1, 7, 6), (24, 25, -8), (-1, 23, -8), (-3, -5, 16), (-14, -10, -15),
        (6, -4, 27), (25, -1, 5), (21, 17, 7), (7, -20, 12), (2, -9, 30),
        (-10, -29, -23), (27, 2, -25), (27, -30, -22), (-7, -21, 13),
        (9, 15, -10), (7, -10, -30), (-21, 26, 28), (15, -30, -8),
        (28, 7, -23), (11, 9, 27), (-5, -17, 4), (24, 25, -11), (-18, 7, 3),
        (28, 14, -9), (21, -3, -4), (-30, -13, 4), (4, 28, -12), (8, -15, -4),
        (-30, -9, -18), (-28, 30, 4), (-17, 21, -20), (8, -3, 9),
        (-20, -29, -12), (-26, 27, 10), (-18, -8, 1), (3, -20, 9),
        (-5, 16, -24), (-24, -30, 25), (-28, 9, -27), (10, 28, 25),
        (-21, 6, 13), (8, 2, 29), (-26, -14, -12), (-20, -13, -18),
        (-4, -8, 25), (8, 1, -16), (-21, -22, -8), (13, -17, 28), (2, -3, 16),
        (27, -10, 21), (23, 18, 26), (6, 8, -7), (21, -22, 11), (2, -30, 25),
        (29, 5, 24), (-14, 28, -30), (5, 10, -4), (-19, -13, 4), (-1, 6, 14),
        (26, -7, 9), (8, 21, 24), (15, -26, -24), (4, 25, 23), (30, -5, 16),
        (-11, -16, -1), (-12, 24, -21), (7, 1, 9), (16, -30, 14),
        (30, 10, -6), (-1, 12, 13), (23, 27, 15), (19, 13, 20),
        (-24, -25, 19), (27, -17, -5), (-2, -16, 23), (1, -16, 29),
        (-19, -7, 27), (-16, 12, 23), (-25, 9, -19), (-24, 27, 10),
        (26, -1, -27), (23, -24, 6), (14, -28, 22), (5, -22, 11),
        (-22, -10, -4), (6, -2, -18), (22, -25, 29), (-4, -21, -3),
        (-6, 7, -25), (22, 10, -18), (-24, 26, 10), (-12, 27, -23),
        (-5, -20, -2), (-26, 28, -2), (-9, 4, 3), (-21, 26, 20), (20, 8, -2),
        (-19, -17, -16), (6, 27, 13), (22, -23, 8), (26, -2, -3),
        (-30, -2, -16), (22, -27, 1), (3, -2, -12), (-24, -25, -26),
        (17, 10, 11), (4, 20, 14), (22, -17, -27), (-12, 16, -3),
        (-25, -20, -10), (23, -16, -5), (3, 30, 15), (25, 28, 5),
        (-24, 22, 23), (13, -14, -30), (-21, -17, 28), (-2, -3, -17),
        (-19, 16, 3), (-6, 26, 1), (9, 27, -18), (21, 17, 29), (30, 8, 7)
    ]

    allreasons = 'select prepro rowgen heur cutgen branch bingo'.split()

    @classmethod
    def solve_sat(self, expression=None, callback=None,
                  return_processor=None, kwargs={}):
        """Runs a solve sat with a callback.  This is similar to the
        other SAT solving procedures in this test suite, with some
        modifications to enable testing of the MIP callback
        functionality.

        expression is the expression argument, and defaults to the
        expression declared in the MIPCallbackTest.

        callback is the callback instance being tested.

        return_processor is a function called with the retval from the
        lp.integer call, and the lp, e.g., return_processor(retval,
        lp), prior to any further processing of the results.

        kwargs is a dictionary of other optional keyword arguments and
        their values which is passed to the lp.integer solution
        procedure."""
        if expression is None:
            expression = self.expression
        if len(expression) == 0:
            return []
        numvars = max(max(abs(v) for v in clause) for clause in expression)
        lp = LPX()
        lp.cols.add(2*numvars)
        for col in lp.cols:
            col.bounds = 0.0, 1.0

        def lit2col(lit):
            if lit > 0:
                return 2*lit-2
            return 2*(-lit)-1

        for i in range(1, numvars+1):
            lp.cols[lit2col(i)].name = 'x_%d' % i
            lp.cols[lit2col(-i)].name = '!x_%d' % i
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
            lp.rows[-1].bounds = 1.0
        for clause in expression:
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
            lp.rows[-1].bounds = 1, None
        retval = lp.simplex()
        if retval is not None:
            return None
        if lp.status != 'opt':
            return None
        for col in lp.cols:
            col.kind = int
        retval = lp.integer(callback=callback, **kwargs)
        if return_processor:
            return_processor(retval, lp)
        if retval is not None:
            return None
        if lp.status != 'opt':
            return None
        return [col.value > 0.99 for col in lp.cols[::2]]

    @classmethod
    def verify(self, expression, assignment):
        """Get the truth of an expression given a variable truth assignment.

        This will return true only if this is a satisfying assignment."""
        # Each clause must be true.
        for clause in expression:
            # For a disjunctive clause to be true, at least one of its
            # literals must be true.
            lits_true = 0
            for lit in clause:
                if (lit > 0) == (assignment[abs(lit)-1]):
                    lits_true += 1
            if lits_true == 0:
                return False
        return True

    def testEmptyCallback(self):
        """Tests for errors with an empty callback."""
        class Callback:
            pass
        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testUncallableCallback(self):
        """Tests that there is an error with an uncallable callback."""
        class Callback:
            default = "Hi, I'm a string!"
        self.Callback = Callback
        with self.assertRaises(TypeError):
            self.solve_sat(callback=self.Callback())

    def testBadCallback(self):
        """Tests that errors in the callback are propagated properly."""
        testobj = self

        class Callback:
            def __init__(self):
                self.stopped = False

            def default(self, tree):
                # We should not be here if we have already made an error.
                testobj.assertFalse(self.stopped)
                self.stopped = True
                1 / 0

        self.Callback = Callback
        with self.assertRaises(ZeroDivisionError):
            self.solve_sat(callback=self.Callback())

    def testCallbackReasons(self):
        """Tests that there are no invalid Tree.reason codes."""
        reasons = set()

        class Callback:
            def default(self, tree):
                reasons.add(tree.reason)

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

        # We should have some items.
        self.assertNotEqual(len(reasons), 0)
        # The reasons should not include anything other than those
        # reasons listed in the allreasons class member.
        self.assertTrue(reasons.issubset(set(self.allreasons)))

    def testCallbackMatchsReasons(self):
        """Ensure that callback methods and reasons are properly matched."""
        testobj = self

        class Callback:
            def select(self, tree): testobj.assertEqual(tree.reason, 'select')

            def prepro(self, tree): testobj.assertEqual(tree.reason, 'prepro')

            def branch(self, tree): testobj.assertEqual(tree.reason, 'branch')

            def rowgen(self, tree): testobj.assertEqual(tree.reason, 'rowgen')

            def heur(self, tree): testobj.assertEqual(tree.reason, 'heur')

            def cutgen(self, tree): testobj.assertEqual(tree.reason, 'cutgen')

            def bingo(self, tree): testobj.assertEqual(tree.reason, 'bingo')

            def default(self, tree): testobj.fail('should not reach default')
        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testCallbackTerminate(self):
        """Tests that termination actually stops the solver."""
        testobj = self

        class Callback:
            def __init__(self):
                self.stopped = False

            def default(self, tree):
                # We should not be here if we have terminated.
                testobj.assertFalse(self.stopped)
                self.stopped = True
                tree.terminate()

        def rp(retval, lp):
            self.assertEqual(retval, 'stop')

        assign = self.solve_sat(callback=Callback(), return_processor=rp)
        self.assertEqual(assign, None)

    def testNodeCountsConsistent(self):
        """Tests that the tree node counts are consistent."""
        testobj = self

        class Callback:
            def __init__(self):
                self.last_total = 0

            def default(self, tree):
                testobj.assertTrue(tree.num_active <= tree.num_all)
                testobj.assertTrue(tree.num_all <= tree.num_total)
                testobj.assertTrue(self.last_total <= tree.num_total)
                testobj.assertEqual(tree.num_active, len(list(tree)))
                self.last_total = tree.num_total

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    @unittest.skipIf(env.version < (4, 21), "TODO")
    def testTreeSelectCallable(self):
        """Test that Tree.select is callable within select, not elsewhere."""
        testobj = self

        class Callback:
            def __init__(self):
                self.last_total = 0

            def select(self, tree):
                tree.select(tree.best_node)

            def default(self, tree):
                try:
                    tree.select(tree.best_node)
                except RuntimeError:
                    return
                else:
                    testobj.fail('Was able to call without error!')

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testSelectCurrNodeNone(self):
        """Test that there is no current node in the select callback."""
        testobj = self

        class Callback:
            def __init__(self):
                self.last_total = 0

            def select(self, tree):
                testobj.assertEqual(tree.curr_node, None)
        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeIterator(self):
        """Test the tree iterator."""
        testobj = self

        class Callback:
            def default(self, tree):
                explicit_actives = []
                n = tree.first_node
                while n:
                    explicit_actives.append(n.subproblem)
                    n = n.next
                actives = [node.subproblem for node in tree]
                testobj.assertEqual(actives, explicit_actives)

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeNodeRichCompare(self):
        testobj = self

        class Callback:
            def default(self, tree):
                n = tree.first_node
                testobj.assertFalse(n == 1)
                testobj.assertFalse(n != 1)
                testobj.assertEqual(NotImplemented, n.__lt__(1))

                testobj.assertTrue(n == n)
                testobj.assertFalse(n != n)
                testobj.assertFalse(n < n)
                testobj.assertFalse(n > n)
                testobj.assertTrue(n <= n)
                testobj.assertTrue(n >= n)

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeNodeAttrs(self):
        testobj = self
        tree_node_fmt_str = \
            "<glpk.TreeNode, active subprob {0:d} of glpk.Tree 0x{1:02x}"

        class Callback:
            def default(self, tree):
                n = tree.first_node
                testobj.assertTrue(n.active)
                testobj.assertGreaterEqual(n.level, 0)
                if n.level == 0:
                    testobj.assertTrue(n.up is None)
                else:
                    testobj.assertFalse(n.up is None)

                best = tree.best_node
                testobj.assertGreaterEqual(n.bound, best.bound)

                testobj.assertIn(
                    tree_node_fmt_str.format(n.subproblem, id(tree)),
                    str(n)
                )

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeSelect(self):
        testobj = self

        class Callback:
            def select(self, tree):
                testobj.assertTrue(tree.select(tree.first_node) is None)

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeBranch(self):
        testobj = self
        self.select_flags = cycle([None, b'D', b'U', b'N'])

        class Callback:
            def default(self, tree):
                if tree.reason != 'branch':
                    with testobj.assertRaises(RuntimeError) as cm:
                        tree.can_branch(1)
                    testobj.assertIn(
                        "function may only be called during branch phase",
                        str(cm.exception)
                    )
                    with testobj.assertRaises(RuntimeError) as cm:
                        tree.branch_upon(1)
                    testobj.assertIn(
                        "function may only be called during branch phase",
                        str(cm.exception)
                    )

            def branch(self, tree):
                testobj.assertEqual(type(tree.can_branch(1)), bool)

                with testobj.assertRaises(IndexError) as cm:
                    tree.can_branch(0)
                testobj.assertIn(
                    "index 0 out of bound for 60 columns",
                    str(cm.exception)
                )

                with testobj.assertRaises(IndexError) as cm:
                    tree.can_branch(61)
                testobj.assertIn(
                    "index 61 out of bound for 60 columns",
                    str(cm.exception)
                )

                with testobj.assertRaises(TypeError) as cm:
                    tree.branch_upon({})
                testobj.assertIn(
                    "an integer is required",
                    str(cm.exception)
                )

                with testobj.assertRaises(TypeError) as cm:
                    tree.branch_upon(1, {})
                if sys.version_info > (3,):
                    err_msg = "must be a byte string of length 1, not dict"
                else:
                    err_msg = "must be char, not dict"
                testobj.assertIn(
                    err_msg,
                    str(cm.exception)
                )

                with testobj.assertRaises(IndexError) as cm:
                    tree.branch_upon(0)
                testobj.assertIn(
                    "index 0 out of bound for 60 columns",
                    str(cm.exception)
                )
                with testobj.assertRaises(IndexError) as cm:
                    tree.branch_upon(61)
                testobj.assertIn(
                    "index 61 out of bound for 60 columns",
                    str(cm.exception)
                )
                # find a column that we can't branch on and then try to
                # branch on it
                try:
                    idx = next(
                        i for i in range(1, 61) if not tree.can_branch(i)
                    )
                    with testobj.assertRaises(RuntimeError) as cm:
                        tree.branch_upon(idx)
                    testobj.assertIn(
                        "cannot branch upon this column",
                        str(cm.exception)
                    )
                except StopIteration:
                    pass

                branch_idx = next(
                    i for i in range(1, 61) if tree.can_branch(i)
                )
                with testobj.assertRaises(ValueError) as cm:
                    tree.branch_upon(branch_idx, b'x')
                testobj.assertIn(
                    "select argument must be D, U, or N",
                    str(cm.exception)
                )

                select_flag = next(testobj.select_flags)
                if select_flag is None:
                    tree.branch_upon(branch_idx)
                else:
                    tree.branch_upon(branch_idx, select_flag)

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeHeuristic(self):
        testobj = self

        class Callback:
            def default(self, tree):
                if tree.reason != 'heur':
                    with testobj.assertRaises(RuntimeError) as cm:
                        tree.heuristic(1)
                    testobj.assertIn(
                        "function may only be called during heur phase",
                        str(cm.exception)
                    )

            def heur(self, tree):
                with testobj.assertRaises(TypeError) as cm:
                    tree.heuristic(1)
                testobj.assertIn(
                    "'int' object is not iterable",
                    str(cm.exception)
                )
                # FIXME: this iterator has only 1 object
                with testobj.assertRaises(ValueError) as cm:
                    tree.heuristic([1])
                testobj.assertIn(
                    "iterator had only 2 objects, but 60 required",
                    str(cm.exception)
                )
                with testobj.assertRaises(TypeError) as cm:
                    tree.heuristic(['a'])
                testobj.assertIn(
                    "iterator must return floats",
                    str(cm.exception)
                )

                assignment = [
                    0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                    1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0,
                    1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
                    0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                    1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0
                ]
                accepted = tree.heuristic(assignment)
                testobj.assertEqual(type(accepted), bool)
                testobj.assertTrue(accepted)

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

    def testTreeGap(self):
        testobj = self
        self.old_gap = float('inf')

        class Callback:
            def default(self, tree):
                new_gap = tree.gap
                testobj.assertEqual(type(new_gap), float)
                testobj.assertGreaterEqual(new_gap, 0.0)
                testobj.assertLessEqual(new_gap, testobj.old_gap)
                testobj.old_gap = new_gap

        assign = self.solve_sat(callback=Callback())
        self.assertTrue(self.verify(self.expression, assign))

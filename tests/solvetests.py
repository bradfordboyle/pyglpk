"""Tests for the solver itself."""

from testutils import *

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
        for c in lp.cols: c.bounds=0,1
        lp.obj[:] = [1,1]
        lp.obj.maximize = True
        lp.rows[0].matrix = [0.5, 1.0]
        lp.rows[0].bounds = None, 1

    def testSimplex(self):
        """Tests solving the simple problem with simplex."""
        self.lp.simplex()
        self.assertAlmostEqual(self.lp.cols['x'].value, 1.0)
        self.assertAlmostEqual(self.lp.cols['y'].value, 0.5)

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

class TwoDimensionalTest(unittest.TestCase):
    def setUp(self):
        self.lp = LPX()

    def testEvolvingConstraintsSimplex(self):
        """Test repeatedly simplex solving an LP with evolving constraints."""
        lp = self.lp
        # Set up the rules of the problem.
        lp.cols.add(2)
        lp.obj[0,1] = 1
        # Try very simple rules.
        x1, x2 = lp.cols[0], lp.cols[1] # For convenience...
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
        self.assertEqual(None, lp.simplex()) # No error?
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
        x2.bounds = 2, None # Instead of x2<=2, must now be >=2!  Tee hee.
        self.assertEqual(None, lp.simplex())
        self.assertEqual('nofeas', lp.status)
        # By removing the first row constraint, we allow opt point (-1,4).
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
        lp.obj[0,1] = 1
        # Try very simple rules.
        x1, x2 = lp.cols[0], lp.cols[1] # For convenience...
        x1.name, x2.name = 'x1', 'x2'
        x1.bounds = None, 1
        x2.bounds = None, 2
        lp.obj.maximize = True
        # This should fault, since interior point methods need some
        # rows, and some columns.
        self.assertEqual('fault', lp.interior())
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
        x2.bounds = 3, None # Instead of x2<=2, must now be >=3!  Tee hee.
        self.assertEqual('nofeas', lp.interior())
        # By removing the first row constraint, we allow opt point (-1,4).
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
        if len(expression)==0: return []
        numvars = max(max(abs(v) for v in clause) for clause in expression)
        lp = LPX()
        lp.cols.add(2*numvars)
        for col in lp.cols:
            col.bounds = 0.0, 1.0
        def lit2col(lit):
            if lit>0: return 2*lit-2
            return 2*(-lit)-1
        for i in xrange(1, numvars+1):
            lp.cols[lit2col( i)].name =  'x_%d'%i
            lp.cols[lit2col(-i)].name = '!x_%d'%i
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
            lp.rows[-1].bounds = 1.0
        for clause in expression:
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
            lp.rows[-1].bounds = 1, None
        retval = lp.simplex()
        if retval != None: return None
        if lp.status != 'opt': return None
        for col in lp.cols:
            col.kind = int
        retval = lp.integer()
        if retval != None: return None
        if lp.status != 'opt': return None
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
                if (lit>0) == (assignment[abs(lit)-1]):
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
        self.failUnless(self.verify(exp, solution))

    def testInsolvableSat(self):
        """Test that an unsolvable satisfiability problem can't be solved."""
        exp = [(1,2), (1,-2), (-1,2), (-1,-2)]
        solution = self.solve_sat(exp)
        self.assertEqual(solution, None)

class MIPCallbackTest(Runner, unittest.TestCase):
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
        (-19, 16, 3), (-6, 26, 1), (9, 27, -18), (21, 17, 29), (30, 8, 7)]

    allreasons='select prepro rowgen heur cutgen branch bingo'.split()

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
        if expression==None: expression=self.expression
        if len(expression)==0: return []
        numvars = max(max(abs(v) for v in clause) for clause in expression)
        lp = LPX()
        lp.cols.add(2*numvars)
        for col in lp.cols:
            col.bounds = 0.0, 1.0
        def lit2col(lit):
            if lit>0: return 2*lit-2
            return 2*(-lit)-1
        for i in xrange(1, numvars+1):
            lp.cols[lit2col( i)].name =  'x_%d'%i
            lp.cols[lit2col(-i)].name = '!x_%d'%i
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
            lp.rows[-1].bounds = 1.0
        for clause in expression:
            lp.rows.add(1)
            lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
            lp.rows[-1].bounds = 1, None
        retval = lp.simplex()
        if retval != None: return None
        if lp.status != 'opt': return None
        for col in lp.cols:
            col.kind = int
        retval = lp.integer(callback=callback, **kwargs)
        if return_processor: return_processor(retval, lp)
        if retval != None: return None
        if lp.status != 'opt': return None
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
                if (lit>0) == (assignment[abs(lit)-1]):
                    lits_true += 1
            if lits_true == 0:
                return False
        return True

    def testEmptyCallback(self):
        """Tests for errors with an empty callback."""
        class Callback:
            pass
        assign = self.solve_sat(callback=Callback())
        self.failUnless(self.verify(self.expression, assign))

    def testUncallableCallback(self):
        """Tests that there is an error with an uncallable callback."""
        class Callback:
            default = "Hi, I'm a string!"
        self.Callback = Callback
        self.assertRaises(TypeError, self.runner,
                          'self.solve_sat(callback=self.Callback())')

    def testBadCallback(self):
        """Tests that errors in the callback are propagated properly."""
        testobj = self
        class Callback:
            def __init__(self):
                self.stopped = False
            def default(self, tree):
                # We should not be here if we have already made an error.
                testobj.failIf(self.stopped)
                self.stopped = True
                a = 1 / 0
        self.Callback = Callback
        self.assertRaises(ZeroDivisionError, self.runner,
                          'self.solve_sat(callback=self.Callback())')

    def testCallbackReasons(self):
        """Tests that there are no invalid Tree.reason codes."""
        reasons = set()
        class Callback:
            def default(self, tree):
                reasons.add(tree.reason)
        assign = self.solve_sat(callback=Callback())
        
        # We should have some items.
        self.assertNotEqual(len(reasons), 0)
        # The reasons should not include anything other than those
        # reasons listed in the allreasons class member.
        self.failUnless(reasons.issubset(set(self.allreasons)))

    def testCallbackMatchsReasons(self):
        """Ensure that callback methods and reasons are properly matched."""
        testobj = self
        class Callback:
            def select(self, tree): testobj.assertEqual(tree.reason, 'select')
            def prepro(self, tree): testobj.assertEqual(tree.reason, 'prepro')
            def branch(self, tree): testobj.assertEqual(tree.reason, 'branch')
            def rowgen(self, tree): testobj.assertEqual(tree.reason, 'rowgen')
            def heur  (self, tree): testobj.assertEqual(tree.reason, 'heur')
            def cutgen(self, tree): testobj.assertEqual(tree.reason, 'cutgen')
            def bingo (self, tree): testobj.assertEqual(tree.reason, 'bingo')
            def default(self,tree): testobj.fail('should not reach default')
        assign = self.solve_sat(callback=Callback())

    def testCallbackTerminate(self):
        """Tests that termination actually stops the solver."""
        testobj = self
        class Callback:
            def __init__(self):
                self.stopped = False
            def default(self, tree):
                # We should not be here if we have terminated.
                testobj.failIf(self.stopped)
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
                self.last_total=0
            def default(self, tree):
                testobj.failUnless(tree.num_active <= tree.num_all)
                testobj.failUnless(tree.num_all <= tree.num_total)
                testobj.failUnless(self.last_total <= tree.num_total)
                testobj.assertEqual(tree.num_active, len(list(tree)))
                self.last_total = tree.num_total
        assign = self.solve_sat(callback=Callback())

    def testTreeSelectCallable(self):
        """Test that Tree.select is callable within select, not elsewhere."""
        if env.version<(4,21): return
        testobj = self
        class Callback:
            def __init__(self):
                self.last_total=0
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

    def testSelectCurrNodeNone(self):
        """Test that there is no current node in the select callback."""
        testobj = self
        class Callback:
            def __init__(self):
                self.last_total=0
            def select(self, tree):
                testobj.assertEqual(tree.curr_node, None)
        assign = self.solve_sat(callback=Callback())

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
                actives = [n.subproblem for n in tree]
                testobj.assertEqual(actives, explicit_actives)
                

# Callbacks did not exist prior to 4.20 anyway.
if env.version<(4,20): del MIPCallbackTest

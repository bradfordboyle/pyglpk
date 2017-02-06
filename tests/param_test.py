"""Tests for setting parameters."""


from glpk import env, LPX
import random
import sys
import unittest


class ParamsTestCase(unittest.TestCase):
    """Tests for setting and getting parameters of the LP."""
    def setUp(self):
        self.lp = LPX()
        param_names = [n for n in dir(self.lp.params) if n[0] != '_']
        param_names.remove('reset')
        self.param_name2default = dict([
            (n, getattr(self.lp.params, n)) for n in param_names])

        # INTEGER PARAMETER VALUES

        # Have nice lists of, for each parameter, valid values, and
        # values illegal because they are of bad type or value.
        p2v, p2bt, p2bv = {}, {}, {}
        # These have legal values 0, 1
        for n in 'price'.split():
            p2v[n] = 0, 1
            p2bv[n] = -100, -1, 2, 3, 4, 5
        # These have legal values 0, 1,2
        for n in 'branch mpsobj'.split():
            p2v[n] = 0, 1, 2
        # These have legal values 0, 1,2, 3
        for n in 'msglev scale btrack'.split():
            p2v[n] = 0, 1, 2, 3
        # These all have the following illegal typed values.
        illegals = 'hi!', [1], {2: 5}, 0.5, complex(1, 0), self.lp
        for n in p2v:
            p2bt[n] = illegals
        # Set their illegal values.
        illegal_vals = -100, -1, 2, 3, 4, 5, 1000
        for n in p2v:
            legals = set(p2v[n])
            p2bv[n] = tuple([v for v in illegal_vals if v not in legals])
        # These may be any positive integer.
        for n in 'outfrq'.split():
            p2v[n] = 1, 2, 3, 100, 1600, 80000, 4000000, 2000000000
            p2bt[n] = illegals
            p2bv[n] = 0, -1, -2, -3, -100, -100000
        # These may be any integer.
        for n in 'itlim'.split():
            p2v[n] = illegal_vals
            p2bt[n] = illegals
        # Integer in range 255.
        for n in 'usecuts'.split():
            p2v[n] = range(0, 0x100)
            p2bt[n] = illegals
            p2bv[n] = tuple(range(-10, 0)) + tuple(range(0x100, 0x110))

        # FLOAT PARAMETER VALUES

        # Previous illegal types included floats.
        illegals = tuple([iv for iv in illegals if type(iv) != float])
        # Floats without bound.
        for n in 'objll objul outdly tmlim'.split():
            p2v[n] = -1e100, -5.23e20, -2.231, -1, 0, 0.5, 1, 3.14159, 1e100
            p2bt[n] = illegals
        # Bounded in [0, 1] range.
        for n in 'relax'.split():
            p2v[n] = 0, .2, .45678, .901, 1
            p2bv[n] = -5e6, -1, -0.01, 1.01, 1.5, 1000
            p2bt[n] = illegals
        # Bounded in double-epsilon to 0.001 range.
        for n in 'tolbnd toldj tolint tolobj tolpiv'.split():
            p2v[n] = 2.3e-16, 1e-8, 0.000523, 0.001
            p2bv[n] = -1000, -1e-8, 0, 0.0011, 0.1, 5.123, 1e100
            p2bt[n] = illegals

        # BOOLEAN PARAMETER VALUES

        # These have boolean values.  They have no illegal values
        # owing to PyGLPK's pecularity of having boolean values set to
        # the bool(value) of value, which is always defined.
        bname = 'dual mpsfree mpsinfo mpsorig mpsskip mpswide presol round'
        for n in bname.split():
            p2v[n] = False, True

        # Set up the mapping from parameter names to tuples of legal
        # values, and illegal params because of value and type.
        self.param_name2values = p2v
        self.param_name2bad_values = p2bv
        self.param_name2bad_type_values = p2bt

    def testIterationCountReadOnly(self):
        """The itcnt parameter should be read only."""
        # TypeError was raised in pre-2.5 versions of Python.
        # AttributeError is raised in 2.5-post versions of Python.
        with self.assertRaises(Exception):
            self.lp.params.itcnt = 5
        with self.assertRaises(Exception):
            self.lp.params.itcnt = -300

    def testDefaultSetIsNotIllegal(self):
        """Sets all parameters to their default values."""
        p2dv = dict(self.param_name2default)
        del p2dv['itcnt']
        for pname, pvalue in p2dv.items():
            setattr(self.lp.params, pname, pvalue)

    def listOfParameterPairs(self, pname2pvalues):
        """Reduce a parameter name, value-sequence mapping to a list.

        Given a mapping from parameter names to a parameter value
        sequence, construct a list of all parameter names and
        parameter values."""
        pp = [(name, value) for name, values in pname2pvalues.items()
              for value in values]
        return pp

    def testSetValues(self):
        """Set many parameter values."""
        pp = self.listOfParameterPairs(self.param_name2values)
        random.Random(2).shuffle(pp)
        for n, v in pp:
            setattr(self.lp.params, n, v)
            self.assertEqual(getattr(self.lp.params, n), v)

    def testSetValuesThenReset(self):
        """Set many parameter values, then use reset() to set defaults."""
        pp = self.listOfParameterPairs(self.param_name2values)
        pp = [(n, v) for n, v in pp if self.param_name2default[n] != v]
        random.Random(3).shuffle(pp)
        for n, v in pp:
            setattr(self.lp.params, n, v)
        self.lp.params.reset()
        for n, v in sorted(self.param_name2default.items()):
            self.assertEqual(getattr(self.lp.params, n), v)

    def testSetBadValues(self):
        """Set many parameters values of bad value."""
        pp = self.listOfParameterPairs(self.param_name2bad_values)
        random.Random(4).shuffle(pp)
        for n, v in pp:
            self.assertRaises(ValueError, setattr, self.lp.params, n, v)

    def testSetBadTypeValues(self):
        """Set many parameters values of bad type."""
        pp = self.listOfParameterPairs(self.param_name2bad_type_values)
        random.Random(5).shuffle(pp)
        for n, v in pp:
            self.assertRaises(TypeError, setattr, self.lp.params, n, v)


try:
    Params
except NameError:
    del ParamsTestCase


class IntegerProgramTestCase(unittest.TestCase):
    """Tests for setting the program type (continuous/int)."""
    def setUp(self):
        self.lp = LPX()

    def testDefaultKind(self):
        """Test that the default kind is float (continuous)."""
        self.assertEqual(self.lp.kind, float)
        self.lp.cols.add(5)
        self.assertEqual([c.kind for c in self.lp.cols], [float]*5)
        self.lp.rows.add(4)
        self.assertEqual([r.kind for r in self.lp.rows], [float]*4)

    def testSettingKind(self):
        """Test that the kind of the problem is not mutable."""
        target_error = AttributeError
        if sys.hexversion < 0x02050000:
            target_error = TypeError
        self.assertRaises(target_error, setattr, self.lp, 'kind', int)
        self.assertRaises(target_error, setattr, self.lp, 'kind', float)

    def testSettingColumnsInt(self):
        """Test setting columns as int."""
        self.lp.cols.add(5)
        for c in [-1, 1, 2]:
            self.lp.cols[c].kind = int
        self.assertEqual([c.kind for c in self.lp.cols],
                         [float, int, int, float, int])

    def testIntAndBinaryCounters(self):
        """Test nint and nbin counters."""
        self.lp.cols.add(5)

        # Convenience function to check the values of nint and nbin.
        def nn(ni, nb):
            self.assertEqual((self.lp.nint, self.lp.nbin), (ni, nb))

        # Do a series of ops and watch how nint and nbin changes.
        nn(0, 0)
        self.lp.cols[0].kind = int
        nn(1, 0)
        self.lp.cols[4].kind = int
        nn(2, 0)
        self.lp.cols[4].bounds = 0, 2
        nn(2, 0)
        self.lp.cols[-1].kind = int
        nn(2, 0)
        self.lp.cols[0].kind = float
        nn(1, 0)
        self.lp.cols[3].kind = int
        nn(2, 0)
        self.lp.cols[4].bounds = 0, 1
        nn(2, 1)
        self.lp.cols[1].kind = int
        nn(3, 1)
        self.lp.cols[3].bounds = 0, 1
        nn(3, 2)
        self.lp.cols[4].bounds = None
        nn(3, 1)
        del self.lp.cols[3]
        nn(2, 0)
        self.lp.cols[2].bounds = 0, 1
        nn(2, 0)
        self.lp.cols[2].kind = int
        nn(3, 1)

    def testIntBounds(self):
        """Tests setting various rows and columns as integer or boolean."""
        self.lp.cols.add(3)
        x, y, z = tuple(self.lp.cols)

        def tk(*targetkinds):
            """Test kind convenience function."""
            self.assertEqual(targetkinds, tuple(c.kind for c in self.lp.cols))
            if set(targetkinds) == set([float]):
                pk = float
            else:
                pk = int
            self.assertEqual(pk, self.lp.kind)

        tk(float, float, float)
        x.kind = int
        x.bounds = 0, 5
        tk(int, float, float)
        y.kind = bool
        tk(int, bool, float)
        y.bounds = 0, 4
        tk(int, int, float)
        x.bounds = 0, 1
        tk(bool, int, float)
        x.kind = float
        tk(float, int, float)
        y.kind = float
        tk(float, float, float)

    def testSettingsRowsInt(self):
        """Test setting rows as int, which should always fail."""
        self.lp.rows.add(5)
        with self.assertRaises(ValueError):
            self.lp.rows[2].kind = int
        with self.assertRaises(ValueError):
            self.lp.rows[4].kind = int

    def testSettingsRowsWithBadObject(self):
        """Test setting columns as something neither int or float."""
        self.lp.cols.add(5)
        for kind in [complex, 'belay', 2, {1: 3}]:
            with self.assertRaises(ValueError):
                self.lp.cols[2].kind = kind


class SimplexControlParametersTestCase(unittest.TestCase):
    """Tests for the simplex method's control parameters."""
    def setUp(self):
        lp = self.lp = LPX()
        lp.rows.add(1)
        lp.cols.add(2)
        for c in lp.cols:
            c.bounds = 0, 1
        lp.obj[:] = [1, 1]
        lp.obj.maximize = True
        lp.rows[0].matrix = [0.5, 1.0]
        lp.rows[0].bounds = None, 1
        self.num_illegals = 100

    def runValueErrorTest(self, param_name, legals):
        """Runs a test of valid values."""
        valid_values = set(legals)
        # Start with "boundary" values.
        values = [min(valid_values)-1, max(valid_values)+1]
        # Come up with a lot of illegal values.
        while len(values) < self.num_illegals:
            randint = random.randint(-20000, 20000)
            if randint not in valid_values:
                values.append(randint)
        for value in values:
            with self.assertRaises(ValueError):
                self.lp.simplex(**{param_name: value})

    def testMessageLevel(self):
        """Test the msg_lev parameter."""
        # For obvious reasons, we only try no output unfortunately.
        self.lp.simplex(msg_lev=LPX.MSG_OFF)

    def testMessageLevelValueErrors(self):
        """Test whether illegal values for msg_lev throw exceptions."""
        self.runValueErrorTest("msg_lev", [
            LPX.MSG_OFF, LPX.MSG_ERR,
            LPX.MSG_ON, LPX.MSG_ALL
        ])

    def testSimplexMethod(self):
        """Test the meth parameter."""
        if env.version >= (4, 31):
            legals = LPX.PRIMAL, LPX.DUAL, LPX.DUALP
        else:
            legals = LPX.PRIMAL, LPX.DUALP
        for p in legals:
            self.lp.simplex(meth=p)

    def testSimplexMethodValueErrors(self):
        """Test whether illegal values for meth throw exceptions."""
        if env.version >= (4, 31):
            legals = LPX.PRIMAL, LPX.DUAL, LPX.DUALP
        else:
            legals = LPX.PRIMAL, LPX.DUALP
        self.runValueErrorTest("meth", legals)

    def testPricingTechnique(self):
        """Test the pricing parameter."""
        for p in (LPX.PT_STD, LPX.PT_PSE):
            self.lp.simplex(pricing=p)

    def testPricingTechniqueValueErrors(self):
        """Test whether illegal values for pricing throw exceptions."""
        self.runValueErrorTest("pricing", [
            LPX.PT_STD, LPX.PT_PSE])

    def testRatioTestTechnique(self):
        """Test the r_test parameter."""
        for p in (LPX.RT_STD, LPX.RT_HAR):
            self.lp.simplex(pricing=p)

    def testRatioTestTechniqueValueErrors(self):
        """Test whether illegal values for r_test throw exceptions."""
        self.runValueErrorTest("r_test", [
            LPX.RT_STD, LPX.RT_HAR])

    def testTolerancePrimalFeasible(self):
        """Test the tol_bnd parameter."""
        for p in (1e-6, 1e-7, 1e-1, .99):
            self.lp.simplex(tol_bnd=p)

    def testTolerancePrimalFeasibleValueErrors(self):
        """Test whether illegal values for tol_bnd throw exceptions."""
        for p in (-1, -1e6, 0, 1, 1e1, 1e5):
            with self.assertRaises(ValueError):
                self.lp.simplex(tol_bnd=p)

    def testToleranceDualFeasible(self):
        """Test the tol_dj parameter."""
        for p in (1e-6, 1e-7, .99):
            self.lp.simplex(tol_dj=p)

    def testToleranceDualFeasibleValueErrors(self):
        """Test whether illegal values for tol_dj throw exceptions."""
        for p in (-1, -1e6, 0, 1, 1e1, 1e5):
            with self.assertRaises(ValueError):
                self.lp.simplex(tol_dj=p)

    def testTolerancePivotalFeasible(self):
        """Test the tol_piv parameter."""
        for p in (1e-6, 1e-7, 1e-1, .99):
            self.lp.simplex(tol_piv=p)

    def testTolerancePivotalFeasibleValueErrors(self):
        """Test whether illegal values for tol_piv throw exceptions."""
        for p in (-1, -1e6, 0, 1, 1e1, 1e5):
            with self.assertRaises(ValueError):
                self.lp.simplex(tol_piv=p)

    def testObjectiveLowerLimit(self):
        """Test the obj_ll parameter."""
        for p in (-200, -100, 0, 3.14159, 100, 200):
            self.lp.simplex(obj_ll=p)

    def testObjectiveLowerLimitTypeErrors(self):
        """Test whether illegal types for obj_ll throw exceptions."""
        for p in ("foo", {"bar": "biz"}, set("baz"), complex(2, 3)):
            with self.assertRaises(TypeError):
                self.lp.simplex(obj_ll=p)

    def testObjectiveUpperLimit(self):
        """Test the obj_ul parameter."""
        for p in (-200, -100, 0, 3.14159, 100, 200):
            self.lp.simplex(obj_ul=p)

    def testObjectiveUpperLimitTypeErrors(self):
        """Test whether illegal types for obj_ul throw exceptions."""
        for p in ("foo", {"bar": "biz"}, set("baz"), complex(2, 3)):
            with self.assertRaises(TypeError):
                self.lp.simplex(obj_ul=p)

    def testIterationLimit(self):
        """Test the it_lim parameter."""
        for p in (0, 100, 200, 1000):
            self.lp.simplex(it_lim=p)

    def testIterationLimitValueErrors(self):
        """Test whether illegal values for it_lim throw exceptions."""
        for p in (-1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.simplex(it_lim=p)

    def testTimeLimit(self):
        """Test the tm_lim parameter."""
        for p in (0, 100, 200, 1000):
            self.lp.simplex(tm_lim=p)

    def testTimeLimitValueErrors(self):
        """Test whether illegal values for tm_lim throw exceptions."""
        for p in (-1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.simplex(tm_lim=p)

    def testOutputFrequency(self):
        """Test the out_frq parameter."""
        for p in (1, 100, 200, 1000):
            self.lp.simplex(out_frq=p)

    def testOutputFrequencyValueErrors(self):
        """Test whether illegal values for out_frq throw exceptions."""
        for p in (0, -1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.simplex(out_frq=p)

    def testOutputDelay(self):
        """Test the out_dly parameter."""
        for p in (0, 1, 100, 200, 1000):
            self.lp.simplex(out_dly=p)

    def testOutputDelayValueErrors(self):
        """Test whether illegal values for out_dly throw exceptions."""
        for p in (-1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.simplex(out_dly=p)

    def testPresolver(self):
        """Test the presolve parameter."""
        self.lp.simplex(presolve=False)
        self.lp.simplex(presolve=True)

    def testPresolverTypeErrors(self):
        """Test whether illegal types for obj_ll throw exceptions."""
        for p in ("foo", {"bar": "biz"}, set("baz"), complex(2, 3)):
            with self.assertRaises(TypeError):
                self.lp.simplex(presolve=p)


@unittest.skipIf(
    env.version < (4, 20),
    'Integer control parameters did not exist prior to GLPK 4.20.'
)
class IntegerControlParametersTestCase(unittest.TestCase):
    """Tests for the simplex method's control parameters.

    Note that one control parameter, callback, is not tested here, but
    rather in the solve test suite.  For GLPK 4.33 many of these options
    are not really tested since GLPK 4.33 appears to be seriously broken
    with respect to parameter setting."""
    def setUp(self):
        lp = self.lp = LPX()
        lp.rows.add(2)
        lp.cols.add(2)
        for c in lp.cols:
            c.bounds = None, None
            c.kind = int
        lp.obj[:] = [1, 1]
        lp.obj.maximize = True
        lp.rows[0].matrix, lp.rows[1].matrix = [2, 1], [1, 2]
        lp.rows[0].bounds, lp.rows[1].bounds = (None, 6.5), (None, 6.5)
        lp.simplex()  # This should have both column vars at 2.1666...
        self.num_illegals = 100

    def runValueErrorTest(self, param_name, legals):
        """Runs a test of valid values."""
        valid_values = set(legals)
        # Start with "boundary" values.
        values = [min(valid_values)-1, max(valid_values)+1]
        # Come up with a lot of illegal values.
        while len(values) < self.num_illegals:
            randint = random.randint(-20000, 20000)
            if randint not in valid_values:
                values.append(randint)
        for value in values:
            with self.assertRaises(ValueError):
                self.lp.integer(**{param_name: value})

    def testMessageLevel(self):
        """Test the msg_lev parameter."""
        # For obvious reasons, we only try no output unfortunately.
        if env.version == (4, 33):
            return
        self.lp.integer(msg_lev=LPX.MSG_OFF)

    def testMessageLevelValueErrors(self):
        """Test whether illegal values for msg_lev throw exceptions."""
        self.runValueErrorTest("msg_lev", [
            LPX.MSG_OFF, LPX.MSG_ERR,
            LPX.MSG_ON, LPX.MSG_ALL
        ])

    def testBranchingTechnique(self):
        """Test the br_tech parameter."""
        if env.version == (4, 33):
            return
        legals = (
            LPX.BR_FFV, LPX.BR_LFV, LPX.BR_MFV,
            LPX.BR_DTH, LPX.BR_PCH
        )
        for p in legals:
            self.lp.integer(br_tech=p)

    def testBranchingTechniqueValueErrors(self):
        """Test whether illegal values for br_tech throw exceptions."""
        self.runValueErrorTest("br_tech", [
            LPX.BR_FFV, LPX.BR_LFV, LPX.BR_MFV,
            LPX.BR_DTH, LPX.BR_PCH
        ])

    def testBacktrackingTechnique(self):
        """Test the bt_tech parameter."""
        if env.version == (4, 33):
            return
        legals = (
            LPX.BT_DFS, LPX.BT_BFS,
            LPX.BT_BLB, LPX.BT_BPH
        )
        for p in legals:
            self.lp.integer(bt_tech=p)

    def testBacktrackingTechniqueValueErrors(self):
        """Test whether illegal values for bt_tech throw exceptions."""
        self.runValueErrorTest("bt_tech", [
            LPX.BT_DFS, LPX.BT_BFS, LPX.BT_BLB, LPX.BT_BPH
        ])

    def testPreprocessingTechnique(self):
        """Test the pp_tech parameter."""
        if env.version < (4, 21) or env.version == (4, 33):
            return
        legals = LPX.PP_NONE, LPX.PP_ROOT, LPX.PP_ALL
        for p in legals:
            self.lp.integer(pp_tech=p)

    def testPreprocessingTechniqueValueErrors(self):
        """Test whether illegal values for pp_tech throw exceptions."""
        if env.version < (4, 21):
            return
        self.runValueErrorTest("pp_tech", [
            LPX.PP_NONE, LPX.PP_ROOT, LPX.PP_ALL])

    def testGomorysMixedCuts(self):
        """Test the gmi_cuts option."""
        if env.version < (4, 24) or env.version == (4, 33):
            return
        legals = True, False
        for p in legals:
            self.lp.integer(gmi_cuts=p)

    def testGomorysMixedCutsTypeErrors(self):
        """Test whether illegal types for gmi_cuts throw exceptions."""
        for p in ("foo", {"bar": "biz"}, set("baz"), complex(2, 3)):
            with self.assertRaises(TypeError):
                self.lp.integer(gmi_cuts=p)

    def testMixedIntegerRoundingCuts(self):
        """Test the mir_cuts option."""
        if env.version < (4, 23) or env.version == (4, 33):
            return
        legals = True, False
        for p in legals:
            self.lp.integer(mir_cuts=p)

    def testMixedIntegerRoundingCutsTypeErrors(self):
        """Test whether illegal types for mir_cuts throw exceptions."""
        for p in ("foo", {"bar": "biz"}, set("baz"), complex(2, 3)):
            with self.assertRaises(TypeError):
                self.lp.integer(mir_cuts=p)

    def testToleranceIntegerFeasible(self):
        """Test the tol_int option."""
        if env.version == (4, 33):
            return
        legals = 1e-7, 1e-5, 1e-3, 1e-1, .99
        for p in legals:
            self.lp.integer(tol_int=p)

    def testToleranceIntegerFeasibleValueErrors(self):
        """Test whether illegal values for tol_obj throw exceptions."""
        for p in (-1, -1e6, 0, 1, 1e1, 1e5):
            with self.assertRaises(ValueError):
                self.lp.integer(tol_int=p)

    def testToleranceObjective(self):
        """Test the tol_obj option."""
        if env.version == (4, 33):
            return
        legals = 1e-7, 1e-5, 1e-3, 1e-1, .99
        for p in legals:
            self.lp.integer(tol_obj=p)

    def testToleranceObjectiveValueErrors(self):
        """Test whether illegal values for tol_obj throw exceptions."""
        for p in (-1, -1e6, 0, 1, 1e1, 1e5):
            with self.assertRaises(ValueError):
                self.lp.integer(tol_obj=p)

    def testTimeLimit(self):
        """Test the tm_lim parameter."""
        if env.version == (4, 33):
            return
        for p in (0, 100, 200, 1000):
            self.lp.integer(tm_lim=p)

    def testTimeLimitValueErrors(self):
        """Test whether illegal values for tm_lim throw exceptions."""
        for p in (-1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.integer(tm_lim=p)

    def testOutputFrequency(self):
        """Test the out_frq parameter."""
        if env.version == (4, 33):
            return
        for p in (1, 100, 200, 1000):
            self.lp.integer(out_frq=p)

    def testOutputFrequencyValueErrors(self):
        """Test whether illegal values for out_frq throw exceptions."""
        for p in (0, -1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.integer(out_frq=p)

    def testOutputDelay(self):
        """Test the out_dly parameter."""
        if env.version == (4, 33):
            return
        for p in (0, 1, 100, 200, 1000):
            self.lp.integer(out_dly=p)

    def testOutputDelayValueErrors(self):
        """Test whether illegal values for out_dly throw exceptions."""
        for p in (-1, -100, -200, -1000000):
            with self.assertRaises(ValueError):
                self.lp.integer(out_dly=p)

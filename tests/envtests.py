"""Tests for the environment."""

# These tests are somewhat different from the other test suites
# insofar as they directly muck with global settings of the underlying
# GLPK library, and thus have some chance of affecting other tests,
# and being affected by the workings of other tests.

from testutils import *
import random, math, gc

class MemoryTestCase(Runner, unittest.TestCase):
    """Tests for the environment memory variables."""
    def setUp(self):
        pass

    def doNottestEmpty(self):
        """Test that there are currently 0 bytes allocated."""
        # This test doesn't really work for some reason.
        gc.collect() # Ensure any leftover LPX objects are cleaned up.
        self.assertEqual(env.blocks, 0)
        self.assertEqual(env.bytes, 0)

    def testPeakIsBound(self):
        """Tests that the peak is always greater than current."""
        self.failUnless(env.blocks <= env.blocks_peak)
        self.failUnless(env.bytes <= env.bytes_peak)

    def testIncrease(self):
        """Test that creating a new LPX increases the memory allocated."""
        gc.collect()
        bytes_first = env.bytes
        lp = LPX()
        lp.rows.add(2)
        lp.cols.add(2)
        lp.name = 'foo'
        self.failUnless(env.bytes > bytes_first)
        del lp
        gc.collect()
        self.assertEqual(env.bytes, bytes_first)

    def testMemLimitNegative(self):
        """Test that negative memory limits are not permitted."""
        if env.version<(4,19): return
        self.assertRaises(ValueError, self.runner, 'env.mem_limit=-1')
        self.assertRaises(ValueError, self.runner, 'env.mem_limit=-500')

    def testMemLimit(self):
        """Test setting the memory limit."""
        if env.version<(4,19): return
        limits = [5,0,10,None,2000]
        self.assertEqual(env.mem_limit, None)
        for limit in limits:
            env.mem_limit = limit
            self.assertEqual(env.mem_limit, limit)
        del env.mem_limit
        self.assertEqual(env.mem_limit, None)

    def testMemLimitType(self):
        """Test that only integer memory limits are permitted."""
        if env.version<(4,19): return
        limits = ['hi!', 3.14159, complex(2,3), {'foo':'bar'}, (23,)]
        for limit in limits:
            self.assertRaises(TypeError, self.runner,
                              'env.mem_limit=%r'%(limit,))

class TerminalTest(unittest.TestCase):
    """Tests for the terminal output settings."""
    def setUp(self):
        # Store the term_on value.
        self.term_on = env.term_on
        # One area where GLPK produces output is in its settings.
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

    def tearDown(self):
        # Restore term_on to whatever value it had when we started.
        env.term_on = self.term_on
        del env.term_hook

    def testTerminalRedirect(self):
        """Test setting the hook function simply."""
        env.term_on = True
        self.char_count = 0
        def count_hook(s):
            self.char_count += len(s)
        env.term_hook = count_hook
        self.lp.simplex(msg_lev=LPX.MSG_ALL)
        self.failUnless(self.char_count > 0)

    def testTerminalOnOff(self):
        """Test setting the terminal on and off."""
        env.term_on = True
        self.char_count = 0
        def count_hook(s):
            import sys
            self.char_count += len(s)
        env.term_hook = count_hook
        self.lp.simplex(msg_lev=LPX.MSG_ALL)
        self.failUnless(self.char_count > 0)
        # Now turn the terminal output off.
        env.term_on = False
        oldcount = self.char_count
        self.lp.simplex(msg_lev=LPX.MSG_ALL)
        if env.version<(4,21) or env.version>(4,26):
            # GLPK versions 4.21 through 4.26 did not fully respect
            # the environment settings.
            self.assertEqual(oldcount, self.char_count)
        # Now turn the terminal output back on.
        env.term_on = True
        self.lp.simplex(msg_lev=LPX.MSG_ALL)
        self.failUnless(self.char_count > oldcount)

    def testMultipleTerminalDirect(self):
        """Test setting the hook function multiple times."""
        env.term_on = True
        numhooks = 3
        self.char_counts = [0]*numhooks
        def makehook(which):
            def thehook(s):
                self.char_counts[which]+=len(s)
            return thehook
        self.hooks = [makehook(i) for i in xrange(numhooks)]
        # Randomly reset the hooks.
        rgen = random.Random(10)
        for trial in xrange(100):
            whichhook = rgen.randint(0, numhooks-1)
            env.term_hook=self.hooks[whichhook]
            old_counts = self.char_counts[:]
            self.lp.simplex(msg_lev=LPX.MSG_ALL)
            # Test that only the count associated with this hook has changed.
            for w in xrange(numhooks):
                if w==whichhook:
                    self.failUnless(self.char_counts[w] > old_counts[w])
                else:
                    self.assertEqual(self.char_counts[w], old_counts[w])

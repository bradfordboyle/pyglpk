"""Tests for the environment."""

# These tests are somewhat different from the other test suites
# insofar as they directly muck with global settings of the underlying
# GLPK library, and thus have some chance of affecting other tests,
# and being affected by the workings of other tests.

import glpk
import random
import gc
import unittest


class MemoryTestCase(unittest.TestCase):
    """Tests for the environment memory variables."""
    def setUp(self):
        pass

    def doNottestEmpty(self):
        """Test that there are currently 0 bytes allocated."""
        # This test doesn't really work for some reason.
        gc.collect()  # Ensure any leftover LPX objects are cleaned up.
        self.assertEqual(glpk.env.blocks, 0)
        self.assertEqual(glpk.env.bytes, 0)

    def testPeakIsBound(self):
        """Tests that the peak is always greater than current."""
        self.assertTrue(glpk.env.blocks <= glpk.env.blocks_peak)
        self.assertTrue(glpk.env.bytes <= glpk.env.bytes_peak)

    def testIncrease(self):
        """Test that creating a new LPX increases the memory allocated."""
        gc.collect()
        bytes_first = glpk.env.bytes
        lp = glpk.LPX()
        lp.rows.add(2)
        lp.cols.add(2)
        lp.name = 'foo'
        self.assertTrue(glpk.env.bytes > bytes_first)
        del lp
        gc.collect()
        self.assertEqual(glpk.env.bytes, bytes_first)

    @unittest.skipIf(glpk.env.version < (4, 19), 'TODO')
    def testMemLimitNegative(self):
        """Test that negative memory limits are not permitted."""
        with self.assertRaises(ValueError):
            glpk.env.mem_limit = -1
        with self.assertRaises(ValueError):
            glpk.env.mem_limit = -500

    @unittest.skipIf(glpk.env.version < (4, 19), 'TODO')
    def testMemLimit(self):
        """Test setting the memory limit."""
        limits = [5, 0, 10, None, 2000]
        self.assertEqual(glpk.env.mem_limit, None)
        for limit in limits:
            glpk.env.mem_limit = limit
            self.assertEqual(glpk.env.mem_limit, limit)
        del glpk.env.mem_limit
        self.assertEqual(glpk.env.mem_limit, None)

    @unittest.skipIf(glpk.env.version < (4, 19), 'TODO')
    def testMemLimitType(self):
        """Test that only integer memory limits are permitted."""
        limits = ['hi!', 3.14159, complex(2, 3), {'foo': 'bar'}, (23,)]
        for limit in limits:
            with self.assertRaises(TypeError):
                glpk.env.mem_limit = limit


class TerminalTest(unittest.TestCase):
    """Tests for the terminal output settings."""
    def setUp(self):
        # Store the term_on value.
        self.term_on = glpk.env.term_on
        # One area where GLPK produces output is in its settings.
        lp = self.lp = glpk.LPX()
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
        # Restore term_on to whatever value it had when we started.
        glpk.env.term_on = self.term_on
        del glpk.env.term_hook

    def testTerminalRedirect(self):
        """Test setting the hook function simply."""
        glpk.env.term_on = True
        self.char_count = 0

        def count_hook(s):
            self.char_count += len(s)

        glpk.env.term_hook = count_hook
        self.lp.simplex(msg_lev=glpk.LPX.MSG_ALL)
        self.assertTrue(self.char_count > 0)

    def testTerminalOnOff(self):
        """Test setting the terminal on and off."""
        glpk.env.term_on = True
        self.char_count = 0

        def count_hook(s):
            self.char_count += len(s)

        glpk.env.term_hook = count_hook
        self.lp.simplex(msg_lev=glpk.LPX.MSG_ALL)
        self.assertTrue(self.char_count > 0)
        # Now turn the terminal output off.
        glpk.env.term_on = False
        oldcount = self.char_count
        self.lp.simplex(msg_lev=glpk.LPX.MSG_ALL)
        if glpk.env.version < (4, 21) or glpk.env.version > (4, 26):
            # GLPK versions 4.21 through 4.26 did not fully respect
            # the environment settings.
            self.assertEqual(oldcount, self.char_count)
        # Now turn the terminal output back on.
        glpk.env.term_on = True
        self.lp.simplex(msg_lev=glpk.LPX.MSG_ALL)
        self.assertTrue(self.char_count > oldcount)

    def testMultipleTerminalDirect(self):
        """Test setting the hook function multiple times."""
        glpk.env.term_on = True
        numhooks = 3
        self.char_counts = [0]*numhooks

        def makehook(which):
            def thehook(s):
                self.char_counts[which] += len(s)
            return thehook

        self.hooks = [makehook(i) for i in range(numhooks)]
        # Randomly reset the hooks.
        rgen = random.Random(10)
        for trial in range(100):
            whichhook = rgen.randint(0, numhooks-1)
            glpk.env.term_hook = self.hooks[whichhook]
            old_counts = self.char_counts[:]
            self.lp.simplex(msg_lev=glpk.LPX.MSG_ALL)
            # Test that only the count associated with this hook has changed.
            for w in range(numhooks):
                if w == whichhook:
                    self.assertTrue(self.char_counts[w] > old_counts[w])
                else:
                    self.assertEqual(self.char_counts[w], old_counts[w])

    def testErrorInTermHook(self):
        """Test that errors in the terminal hook are ignored."""
        glpk.env.term_on = True

        def error_hook(s):
            1.0 / 0.0
        glpk.env.term_hook = error_hook
        self.lp.simplex(msg_lev=glpk.LPX.MSG_ALL)
        self.assertEqual(self.lp.status, 'opt')

    def testGetTermOn(self):
        """Test getting/setting term_on."""
        glpk.env.term_on = True
        self.assertTrue(glpk.env.term_on)
        with self.assertRaises(TypeError) as cm:
            glpk.env.term_on = 'foo'
        self.assertIn('term_on must be set with bool', str(cm.exception))

    def testGetTermHook(self):
        """Test getting the terminal hook."""
        self.assertTrue(glpk.env.term_hook is None)

        def noop_hook(s):
            pass
        glpk.env.term_hook = noop_hook
        self.assertEqual(noop_hook, glpk.env.term_hook)

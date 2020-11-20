"""Tests for the solver itself."""

from glpk import env, LPX
import sys
import unittest
from itertools import cycle


class TableauTest(unittest.TestCase):
    """A simple suite of tests for this problem.

    max (x+y) subject to
    0.5*x + y <= 1
    0 <= x <= 1
    0 <= y <= 1

    This should have optimal solution x=1, y=0.5."""

    def setUp(self):
        lp = self.lp = LPX()
        lp.rows.add(1)
        self.z = lp.rows[0]
        lp.cols.add(2)
        self.x = lp.cols[0]
        self.y = lp.cols[1]
        for c in lp.cols:
            c.bounds = 0, 1
        lp.obj[:] = [1, 1]
        lp.obj.maximize = True
        lp.rows[0].matrix = [0.5, 1.0]
        lp.rows[0].bounds = None, 1
        self.lp.simplex()

    def test_warm_up(self):
        self.assertIsNone(self.lp.warm_up())

    def test_eval_tab_row_non_basic_variable(self):
        """Tests computing a row of the current simplex tableau"""
        expected_row = [(self.z, 1.0), (self.x, -0.5)]
        actual_row = self.y.eval_tab_row()
        self.assertEqual(expected_row, actual_row)

    def test_eval_tab_row_basic_variable(self):
        with self.assertRaises(ValueError) as context:
            self.z.eval_tab_row()

        self.assertIn("variable is non-basic", str(context.exception))

    def test_eval_tab_col_non_basic_variables(self):
        """Tests computing a row of the current simplex tableau"""
        expected_column = [(self.y, 1.0)]
        actual_column = self.z.eval_tab_col()
        self.assertEqual(expected_column, actual_column)

        expected_column = [(self.y, -0.5)]
        actual_column = self.x.eval_tab_col()
        self.assertEqual(expected_column, actual_column)

    def test_eval_tab_col_basic_variable(self):
        with self.assertRaises(ValueError) as context:
            self.y.eval_tab_col()

        self.assertIn("variable is basic", str(context.exception))

    def test_transform_row(self):
        expected_row = [(self.z, 3.0), (self.x, 2.5)]
        actual_row = self.lp.transform_row([(self.x, 4.0), (self.y, 3.0)])
        self.assertEqual(expected_row, actual_row)

    def test_transform_row_with_auxiliary_variable(self):
        with self.assertRaises(ValueError) as context:
            self.lp.transform_row([(self.z, 1.0)])

        self.assertIn("input variables must be structural", str(context.exception))

    def test_transform_row_with_wrong_input_types(self):
        with self.assertRaises(TypeError) as context:
            self.lp.transform_row(1)

        self.assertIn("object is not iterable", str(context.exception))

        with self.assertRaises(TypeError) as context:
            self.lp.transform_row([1, 2, 4])

        self.assertIn("item must be two element tuple", str(context.exception))

        with self.assertRaises(TypeError) as context:
            self.lp.transform_row([(1, 2)])

        self.assertIn("tuple must contain glpk.Bar and double", str(context.exception))

    def test_transform_row_with_variable_from_other_lp(self):
        other_lp = LPX()
        other_lp.cols.add(3)

        with self.assertRaises(ValueError) as context:
            self.lp.transform_row([(other_lp.cols[0], 3.0)])

        self.assertIn("variable not associated with this LPX", str(context.exception))

    def test_transform_col(self):
        expected_col = [(self.y, -4.0)]
        actual_col = self.lp.transform_col([(self.z, 4.0)])
        self.assertEqual(expected_col, actual_col)

    def test_transform_col_with_structural_variable(self):
        with self.assertRaises(ValueError) as context:
            self.lp.transform_col([(self.x, 4.0)])

        self.assertIn("input variables must be auxiliary", str(context.exception))

    def test_transform_col_with_wrong_input_types(self):
        with self.assertRaises(TypeError) as context:
            self.lp.transform_col(1)

        self.assertIn("object is not iterable", str(context.exception))

        with self.assertRaises(TypeError) as context:
            self.lp.transform_col([1, 2, 4])

        self.assertIn("item must be two element tuple", str(context.exception))

        with self.assertRaises(TypeError) as context:
            self.lp.transform_col([(1, 2)])

        self.assertIn("tuple must contain glpk.Bar and double", str(context.exception))

    def test_transform_col_with_variable_from_other_lp(self):
        other_lp = LPX()
        other_lp.rows.add(3)

        with self.assertRaises(ValueError) as context:
            self.lp.transform_col([(other_lp.rows[2], 3.0)])

        self.assertIn("variable not associated with this LPX", str(context.exception))

    def test_prime_ratio_test_increasing_direction(self):
        expected_piv = 0

        actual_piv = self.lp.prime_ratio_test([(self.y, 4.0)], 1, 1e-3)
        self.assertEqual(expected_piv, actual_piv)

    def test_prime_ratio_test_decreasing_direction(self):
        expected_piv = 0
        actual_piv = self.lp.prime_ratio_test([(self.y, 4.0)], -1, 1e-3)
        self.assertEqual(expected_piv, actual_piv)

    def test_prime_ratio_test_invalid_direction(self):
        with self.assertRaises(ValueError) as context:
            self.lp.prime_ratio_test([(self.y, 4.0)], 2, 1e-3)

        self.assertIn(
            "direction must be either +1 (increasing) or -1 (decreasing)",
            str(context.exception),
        )

    def test_dual_ratio_test_increasing_direction(self):
        expected_piv = -1
        bar_input = [(self.z, 4.0), (self.x, 3.0)]
        actual_piv = self.lp.dual_ratio_test(bar_input, 1, 1e-3)
        self.assertEqual(expected_piv, actual_piv)

    def test_dual_ratio_test_decreasing_direction(self):
        expected_piv = 1
        bar_input = [(self.z, 4.0), (self.x, 3.0)]
        actual_piv = self.lp.dual_ratio_test(bar_input, -1, 1e-3)
        self.assertEqual(expected_piv, actual_piv)

    def test_dual_ratio_test_invalid_direction(self):
        with self.assertRaises(ValueError) as context:
            self.lp.dual_ratio_test([], 2, 1e-3)

        self.assertIn(
            "direction must be either +1 (increasing) or -1 (decreasing)",
            str(context.exception),
        )

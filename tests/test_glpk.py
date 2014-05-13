"""The main module for running the tests."""

from testutils import *

errors, failures, total = 0, 0, 0

test_names = ['name', 'obj', 'vector', 'matrix',
              'param', 'memory', 'solve', 'env']

# Run all of the checks.
for name_of_test in test_names:
    mod = __import__(name_of_test + 'tests')
    result = testsRunner(mod)
    errors += len(result.errors)
    failures += len(result.failures)
    total += result.testsRun
# Print the final summary report.
if errors + failures:
    import sys
    print 'TESTS FAILED!! : %d failures, %d errors from %d tests total' % (
        failures, errors, total)
    sys.exit(1)
else:
    print 'TESTS PASSED!!  %d tests total' % total

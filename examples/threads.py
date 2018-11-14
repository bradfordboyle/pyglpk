import sat
import sys
import threading

# At present, GLPK itself, and consequently PyGLPK, are not thread
# safe.  As a result, this "multithreaded" example at present does not
# run the threads in parallel during solving.  I am, however,
# retaining the example since I live in hope that

vars, clauses = 50, 200
num_threads = 8

def solve_problem(tid, num):
    exp = sat.generate_cnf(vars, clauses)
    assignment = sat.solve_sat(exp)
    if assignment: assert sat.verify(exp, assignment)
    sys.stdout.write((('+' if assignment else '-')+'(%d,%2d)')%(tid,num))
    sys.stdout.flush()

def thread_callable(tid):
    for i in range(50):
        solve_problem(tid, i)
    sys.stdout.write('F(%d)' % tid)
    sys.stdout.flush()

threads = []
for tid in range(num_threads):
    thread = threading.Thread(target=thread_callable, args=(tid,))
    threads.append(thread)
    thread.start()

for thread in threads:
    thread.join()

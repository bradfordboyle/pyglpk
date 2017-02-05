=======================
Building and Installing
=======================

Building this module requires that the user have installed the `GLPK
<http://www.gnu.org/software/glpk/>`_ 4.18 or later, and `GMP
<http://gmplib.org/>`_ libraries. The module builds and appears to work on my
simple test files in Python 2.4 and 2.5. Earlier versions of Python may not
work. I have developed this on both OS X and Linux. I do not know if it will
work on Windows.

Here are the brief instructions for installing.

* Build the module using ``make``. (Pay attention to warnings!)

* Run ``make test`` to execute the test suite. You want the last line of output
  to be ``Tests succeeded!``

* Build the module using ``make install`` to install the module in your Python
  build's ``site-packages`` directory. On a typical system, you will need to
  run this as the superuser (probably ``sudo make install``).

In the ideal case (that is, you have GLPK and GMP libraries, headers,
executables and the like installed in default search paths), this will just
work.

---------------
Troubleshooting
---------------

A few details just in case things don't go that easily:

How PyGLPK builds depends on which version of GLPK is installed. GLPK is an
evolving library, and some functionality has been added only since older
versions.

* Perhaps you don't have GLPK or GMP installed, or not the development versions
  with the headers.

* Perhaps your ``INCLUDE_PATH``, ``LIBRARY_PATH``, or ``LD_LIBRARY_PATH`` paths
  are not set correctly.

* Perhaps you have multiple versions of GLPK on your system in different
  locations, and it is confusing the system during building. E.g., it may build
  with one version, and then try to run the program with another.

* Perhaps the GLPK library was installed incorrectly. For example, on my Ubuntu
  box, I cannot build *anything* with the GLPK installed by my Debian package
  manager, not even a simple ``main { LPX*lp=lpx_create_prob();
  lpx_delete_prog(lp); }``, owing to a faulty compile of the libraries.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If You're Having Trouble Building
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below are some suggestions to try or avenues to pursue if you're having trouble
building. This is not an exhaustive list, just "the most likely suspects."

* The build process assumes that the default Python (i.e., the Python you would
  get if you tried executing ``python`` within ``sh``) is the one for which you
  want to build ``glpk``. If this is untrue, modify the ``Makefile`` file to
  change the assignment ``PYTHON:=python`` to something like
  ``PYTHON:=/path/to/other/python``.

* As described earlier, perhaps your environment variable paths are not set
  correctly. Relevant paths are ``INCLUDE_PATH``, ``LIBRARY_PATH``,
  ``LD_LIBRARY_PATH`` (yeah, I know).

* Perhaps you do not actually have a ``glpk.h`` file in the include
  directories. This may be because you have a binary non-developer installation
  of GLPK.

* Perhaps the ``setup.py`` script, in its attempt to do something reasonably
  clever, drew the wrong conclusions about where appropriate libraries and
  headers may be located, or the version of GLPK. (This may happen in certain
  situations where there are multiple builds of GLPK floating around with
  different versions, or your binaries are stored in strange locations.) Within
  ``setup.py`` is a small section beginning with ``USERS DO CUSTOM INSTRUCTIONS
  HERE`` where you can override its "cleverness" by manually defining the
  library name, location of appropriate library and include search directories,
  and other traits.

* Multiple versions of GLPK as described earlier. This may be addressed through
  editing the ``setup.py``

* Faulty installation of GLPK.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If You're Having Trouble Testing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once you have built the module successfully and there is a ``glpk.so`` symbolic
link defined to the shared library, you should run the principle test suite
through ``make test``.

If you're running the test suite through ``make test`` and it throws some sort
of exception, what the problem is depends on what type of error you see.

``ImportError: No module named glpk``
    Ensure that the module actually built.

``ImportError: (stuff) Symbol not found``
  Most likely, it cannot find a required library. Run ``ldd glpk.so`` (on
  Linux) or ``otool -L glpk.so`` (on OS X) to see which libraries it cannot
  find. Use this information to ensure that you environment variable
  ``LD_LIBRARY_PATH`` points to directories containing appropriate libraries.
  (Note that merely finding all libraries is not necessarily sufficient in the
  case where there are multiple GLPK libraries floating around. Ensure you're
  not building with one, but running with the other.)

``AssertionError`` (or other error)
  The good news is that the module appears to load and run and it's throwing
  normal Python exceptions. The bad news is one of my tests failed. This should
  not happen, but if it does, send me a bug report.</dd>

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If You're Having Trouble Installing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If it says permission denied when running ``make install``, perhaps you need to
run the command as the superuser, e.g., ``sudo make install``.

^^^^^^^^^^^^
Uninstalling
^^^^^^^^^^^^

Like many Python modules, PyGLPK builds itself with the aid of Python's
included ``distutils`` module. As of the time of this writing, ``distutils``
does not support uninstallation. I am not comfortable with writing my own
solution. Given that this is a potentially destructive option, there may be
unintended, unfortunate consequences on some person's configuration I did not
anticipate. However, one may still uninstall PyGLPK by importing the ``glpk``
module, and checking the ``glpk.__file__`` attribute. This will tell you where
the module file is stored. Then, you can go to that directory and remove the
module file, and the associated ``egg-info`` file, and rid yourself of PyGLPK.

from distutils.core import setup, Extension
import os
import sys
import versioneer

useparams = False

if 'TOXENV' in os.environ and 'SETUPPY_CFLAGS' in os.environ:
    os.environ['CFLAGS'] = os.environ['SETUPPY_CFLAGS']

sources = 'glpk 2to3 lp barcol bar obj util kkt tree environment'
source_roots = sources.split()
if useparams:
    source_roots.append('params')

# This build process will not work with anything prior to GLPK 4.16,
# since there were many notable changes in GLPK including,
# importantly, something which actually contains the version number.

pkgdirs = [] # incdirs and libdirs get these
libs = ["glpk", "gmp"]
defs = []
incdirs = []
libdirs = []

def append_env(L, e):
    v = os.environ.get(e)
    if v and os.path.exists(v):
        L.append(v)

append_env(pkgdirs, "GLPK")

# Hack up sys.argv
unprocessed = []
for arg in sys.argv[1:]:
    if arg.startswith("--with-glpk="):
        pkgdirs.append(arg.split("=", 1)[1])
        continue
    unprocessed.append(arg)
sys.argv[1:] = unprocessed

for pkgdir in pkgdirs:
    incdirs.append(os.path.join(pkgdir, "include"))
    libdirs.append(os.path.join(pkgdir, "lib"))

defs.append(('VERSION_NUMBER', '"{}"'.format(versioneer.get_version())))
if useparams:
    defs.append(('USEPARAMS', None))

# Now, finally, define that module!
pyglpk_ext = Extension(
    'glpk', [os.path.join('src', r+'.c') for r in source_roots],
    libraries=libs, include_dirs=incdirs,
    library_dirs=libdirs, define_macros=defs,
    runtime_library_dirs=libdirs)

ld = """The PyGLPK module gives one access to the functionality
of the GNU Linear Programming Kit.
"""

setup(
    name='glpk',
    version=versioneer.get_version(),
    cmdclass=versioneer.get_cmdclass(),
    description='PyGLPK, a Python module encapsulating GLPK.',
    long_description=ld,
    author='Thomas Finley',
    author_email='tfinley@gmail.com',
    url='http://tfinley.net/software/pyglpk/',
    maintainer='Bradford D. Boyle',
    maintainer_email='bradford.d.boyle@gmail.com',
    license='GPL',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Operating System :: POSIX',
        'Operating System :: MacOS :: MacOS X',
        'Topic :: Scientific/Engineering :: Mathematics',
        'Topic :: Software Development :: Libraries :: Python Modules'
    ],
    ext_modules=[pyglpk_ext]
)

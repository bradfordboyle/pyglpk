from setuptools import setup, Extension
from setuptools_scm import get_version
import os
import sys

useparams = False

sources = 'glpk 2to3 lp barcol bar obj util kkt tree environment'
source_roots = sources.split()
if useparams:
    source_roots.append('params')

# This build process will not work with anything prior to GLPK 4.16,
# since there were many notable changes in GLPK including,
# importantly, something which actually contains the version number.

pkgdirs = [] # incdirs and libdirs get these
libs = ["glpk"]
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

is_windows = sys.platform.startswith("win")
version_fmt_str = '"{}"' if not is_windows else '"\\"{}\\""'
defs.append(('VERSION_NUMBER', version_fmt_str.format(get_version())))

if useparams:
    defs.append(('USEPARAMS', None))

runtime_library_dirs = libdirs if not is_windows else []

# Now, finally, define that module!
pyglpk_ext = Extension(
    'glpk', [os.path.join('src', r+'.c') for r in source_roots],
    libraries=libs, include_dirs=incdirs,
    library_dirs=libdirs, define_macros=defs,
    runtime_library_dirs=runtime_library_dirs)

setup(
    use_scm_version=True,
    ext_modules=[pyglpk_ext]
)

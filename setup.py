import sys
import platform
from setuptools import setup, Extension
from Cython.Build import cythonize

# Detect platform
is_macos = sys.platform == "darwin"
is_windows = sys.platform == "win32"

# Common sources
sources = [
    "xpress9.pyx",
    "src/Xpress9DLL.c",
    "src/Xpress9DecLz77.c",
    "src/Xpress9EncLz77.c",
    "src/Xpress9DecHuffman.c",
    "src/Xpress9EncHuffman.c",
    "src/Xpress9Misc.c",
]

# Compilation flags
extra_compile_args = ["-O3"]
extra_link_args = []

if not is_macos and not is_windows:
    # Enable OpenMP for Linux only
    extra_compile_args.append("-fopenmp")
    extra_link_args.append("-fopenmp")
elif is_windows:
    # Enable OpenMP for MSVC
    extra_compile_args.append("/openmp")
    extra_compile_args.append("/DBUILD_STATIC")

# Define the extension
xpress9_module = Extension(
    "xpress9",
    sources=sources,
    include_dirs=["include"],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

# Setup
setup(
    name="xpress9",
    ext_modules=cythonize([xpress9_module]),
    zip_safe=False,
)

import sys
import platform
from setuptools import setup, Extension
from Cython.Build import cythonize

# Detect platform
is_macos = sys.platform == "darwin"

# Common sources
sources = [
    "xpress9.pyx",
    "src/Xpress9DLL.c",
    "src/Xpress9DecLz77.c",
    "src/Xpress9DecHuffman.c",
    "src/Xpress9Misc.c",
]

# Compilation flags
extra_compile_args = ["-O3"]
extra_link_args = []

if not is_macos:
    # Enable OpenMP for Linux & Windows
    extra_compile_args.append("-fopenmp")
    extra_link_args.append("-fopenmp")
else:
    # macOS: Use Homebrew's libomp
    omp_include_path = "/opt/homebrew/opt/libomp/include"
    omp_lib_path = "/opt/homebrew/opt/libomp/lib"
    
    extra_compile_args.extend(["-Xpreprocessor", "-fopenmp", f"-I{omp_include_path}"])
    extra_link_args.extend(["-lomp", f"-L{omp_lib_path}"])

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

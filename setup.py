from setuptools import setup, Extension
from Cython.Build import cythonize

xpress9_module = Extension(
    "xpress9",
    sources=[
        "xpress9.pyx",  # Our Cython wrapper
        "src/Xpress9DLL.c",  # Include C source files directly
        "src/Xpress9DecLz77.c",
        "src/Xpress9DecHuffman.c",
        "src/Xpress9Misc.c",
    ],
    include_dirs=["include"],  # Ensure headers are available
    extra_compile_args=["-O3", "-fopenmp"],  # Optimization flags
    extra_link_args=["-fopenmp"],  # Parallel execution support
)

setup(
    name="xpress9",
    ext_modules=cythonize([xpress9_module]),
    zip_safe=False,
)

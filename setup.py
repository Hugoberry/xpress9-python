import sys
import platform
from setuptools import setup, Extension
from Cython.Build import cythonize
import os

# Read README.md for long description
with open("README.md", "r", encoding="utf-8") as f:
    long_description = f.read()

# Detect platform
is_macos = sys.platform == "darwin"
is_windows = sys.platform == "win32"
is_linux = not is_macos and not is_windows

# Common sources
sources = [
    "xpress9.pyx",
    "src/Xpress9Wrapper.c",
    "src/Xpress9DecLz77.c",
    "src/Xpress9EncLz77.c",
    "src/Xpress9DecHuffman.c",
    "src/Xpress9EncHuffman.c",
    "src/Xpress9Misc.c",
]

# Default optimization for all platforms
if is_linux:
    # Linux gets O2 for stability
    optimization_flag = "-O2"
else:
    # Windows and macOS get O3 for performance
    optimization_flag = "-O3" if not is_windows else "/O2"

# Compilation flags
extra_compile_args = [optimization_flag]
extra_link_args = []

if is_linux:
    # Linux-specific flags
    extra_compile_args.extend(["-fopenmp", "-fPIC"])
    extra_link_args.append("-fopenmp")
elif is_windows:
    # Windows-specific flags
    extra_compile_args.extend(["/openmp", "/DBUILD_STATIC"])
elif is_macos:
    # macOS-specific flags if needed
    pass

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
    version="0.3.7",
    description="Python bindings for the Xpress9 compression library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Igor Cotruta",
    author_email="hugoberry314@gmail.com",
    url="https://github.com/Hugoberry/xpress9-python",
    ext_modules=cythonize([xpress9_module]),
    python_requires=">=3.8",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Operating System :: OS Independent",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: System :: Archiving :: Compression",
    ],
    keywords="compression, xpress9, microsoft",
    zip_safe=False,
)
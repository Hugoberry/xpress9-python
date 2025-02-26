from setuptools import setup, Extension, find_packages
import platform
import os
import sys

# Define the list of source files
sources = [
    "xpress9/_xpress9.pyx",
    "xpress9/src/Xpress9DecHuffman.c",
    "xpress9/src/Xpress9DecLz77.c",
    "xpress9/src/Xpress9Misc.c",
]

# Define platform-specific compiler and linker flags
extra_compile_args = []
extra_link_args = []

# MacOS-specific settings for universal2 binary
if platform.system() == "Darwin":
    # Check if ARCHFLAGS is set in the environment
    archflags = os.environ.get("ARCHFLAGS")
    if archflags:
        extra_compile_args.extend([archflags])
        extra_link_args.extend([archflags])
    else:
        # Default to universal2 build if ARCHFLAGS not explicitly set
        extra_compile_args.extend(["-arch", "x86_64", "-arch", "arm64"])
        extra_link_args.extend(["-arch", "x86_64", "-arch", "arm64"])
    
    # Set minimum macOS version
    mac_target = os.environ.get("MACOSX_DEPLOYMENT_TARGET", "11.0")
    extra_compile_args.append(f"-mmacosx-version-min={mac_target}")
    extra_link_args.append(f"-mmacosx-version-min={mac_target}")

# Define the extension module
extensions = [
    Extension(
        "xpress9._xpress9",
        sources=sources,
        include_dirs=["xpress9/src"],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        language="c",
    ),
]

setup(
    name="xpress9-python",
    version="0.1.0",
    description="Python bindings for the Xpress9 compression library",
    author="Igor Cotruta",
    author_email="hugoberry314@gmail.com",
    packages=find_packages(),
    ext_modules=extensions,
    python_requires=">=3.6",
    setup_requires=["cython>=0.29.0"],
    install_requires=[],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Cython",
        "Topic :: System :: Archiving :: Compression",
    ],
)
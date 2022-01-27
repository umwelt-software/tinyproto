#
# Copyright 2021 (C) Alexey Dynda
#
# This file is part of Tiny Protocol Library.
#
# Protocol Library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Protocol Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
#

import glob
from distutils.core import setup, Extension
import platform

source_files =       glob.glob('./src/**/*.c*', recursive=True )
source_files.extend( glob.glob('./python/**/*.cpp', recursive=True ) )

if platform.system() == "Linux":
    libs = ["stdc++"] # Required for Linux and doesn't work in Windows
else:
    libs = []

tinyproto_module = Extension(
    'tinyproto_',
    sources=source_files,
    include_dirs=['./src','./python'],
    define_macros=[("MYDEF", None), ("TINY_FD_DEBUG", 0), ("TINY_LOG_LEVEL_DEFAULT", 0)],
    library_dirs=[],
    libraries=libs,
    language='c++',
)


# It has also scripts argument to build
setup(
    name='tinyproto',
    author='Alexey Dynda',
    author_email='alexey.dynda@gmail.com',
    url='https://github.com/lexus2k/tinyproto',
    license='LGPLv3',
    version='1.0.0',
    description='tinyproto module wrapper',
    package_dir = { "tinyproto": "./python" },
    packages = ['tinyproto', 'tinyproto.wrappers', 'tinyproto.options'],
    # py_modules = ['tinyproto.options'],
    ext_modules=[tinyproto_module],
)


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

source_files =       glob.glob('./src/**/*.c*', recursive=True )
source_files.extend( glob.glob('./python/**/*.cpp', recursive=True ) )

tinyproto_module = Extension(
    'tinyproto',
    sources=source_files,
    include_dirs=['./src','./python'],
    define_macros=[("MYDEF", None)],
    library_dirs=[],
    libraries=["stdc++"],
    language='c++', )


# It has also scripts argument to build
setup(
    name='tinyproto',
    author='Alexey Dynda',
    author_email='alexey.dynda@gmail.com',
    url='https://github.com/lexus2k/tinyproto',
    license='LGPLv3',
    version='0.99.0',
    description='tinyproto module wrapper',
    ext_modules=[tinyproto_module], )


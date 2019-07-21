/*
    Copyright 2018 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __OS_TINY_CONFIG_H__
#define __OS_TINY_CONFIG_H__

#ifdef ARDUINO
#include "arduino/tiny_defines.h"

#elif __linux__
#include "linux/tiny_defines.h"

#elif defined(__XTENSA__)
#include "esp32/tiny_defines.h"

#else
#include "mingw32/tiny_defines.h"

#endif

#endif /* __OS_TINY_CONFIG_H__ */


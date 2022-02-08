/*
    Copyright 2016-2017,2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /// structure defines base type for the lists
    typedef struct list_element_
    {
        /// pointer to the next element in the list
        struct list_element_ *pnext;
        /// pointer to the previous element in the list
        struct list_element_ *pprev;
    } list_element;

    uint16_t tiny_list_add(list_element **head, list_element *element);

    void tiny_list_remove(list_element **head, list_element *element);

    void tiny_list_clear(list_element **head);

    void tiny_list_enumerate(list_element *head, uint8_t (*enumerate_func)(list_element *element, uint16_t data),
                             uint16_t data);

#ifdef __cplusplus
}
#endif

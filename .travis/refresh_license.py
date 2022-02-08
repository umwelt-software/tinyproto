"""
    Copyright 2022 (C) Alexey Dynda

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
"""

import os
import datetime
import hashlib

exclude_paths = [
    './.git',
    './.travis',
    './examples',
]

include_files = [
    '.cpp',
    '.c',
    '.inl',
    '.py',
    '.h',
]

code = []

hash_db = {}

def get_hash(data):
    hash_md5 = hashlib.md5()
    hash_md5.update(data.encode())
    return hash_md5.hexdigest()


def open_hash_db(name):
    if not os.path.exists(name):
        return False
    with open(name) as f:
        lines = f.readlines()
    for l in lines:
        k = l.split(':')[0]
        v = l.split(':')[1].strip()
        hash_db[k] = v
    return True

def save_hash_db(name):
    with open(name, "w") as f:
        for v in hash_db.keys():
            f.write(v + ":" + hash_db[v] + "\n")
    return True

def update_hash_in_db(name, h):
    hash_db[name] = h

def get_hash_from_db(name):
    if name in hash_db:
        return hash_db[name]
    return None

def is_file_modified(name):
    with open(name) as f:
        lines = f.readlines()
    # Calculate hash for the file
    new_hash = str(get_hash(''.join(lines)))
    old_hash = get_hash_from_db(name)
    if old_hash is not None:
        if old_hash != new_hash:
            file_modified = True
        else:
            file_modified = False
    else:
        file_modified = False
    return (file_modified, new_hash)


def check_and_update_license(name, new_license = False):
    with open(name) as f:
        lines = f.readlines()
    (file_modified, new_hash) = is_file_modified(name)
    copyright_exists = False
    for i in range(min(10,len(lines))):
        if "Copyright" in lines[i]:
            now = datetime.datetime.now()
            ccc = "C"
            if "(c)" in lines[i]:
                ccc = "(c)"
            if "Copyright {}".format(ccc) in lines[i]:
                YEAR = lines[i].split("Copyright {}".format(ccc))[1].split(',')[0].strip()
            else:
                YEAR = lines[i].split("Copyright")[1].split(ccc)[0].strip()
            if YEAR.endswith(','):
                YEAR = YEAR[:-1]
            if not YEAR.endswith("{}".format(now.year)):
                if YEAR.endswith("-{}".format(now.year-1)):
                    YEAR = YEAR[:-5]
                    YEAR += "-{}".format(now.year)
                elif YEAR.endswith("{}".format(now.year-1)):
                    YEAR += "-{}".format(now.year)
                else:
                    YEAR += ",{}".format(now.year)
            copyright_exists = True
    insert_position = 0
    if not copyright_exists:
        now = datetime.datetime.now()
        YEAR = "{}".format(now.year)
    else:
        # Cut old license
        start = -1
        end = -1
        for i in range(len(lines) - 1):
            if start < 0 and i > 10:
                copyright_exists = False
                break
            if start < 0 and (lines[i] == "/*\n" or lines[i] == "#\n" or lines[i] == "\"\"\"\n"):
                start = i
                continue
            if start >= 0 and start < 6 and (lines[i] == "*/\n" or lines[i] == " */\n" or
                                            (lines[i] == "#\n" and lines[i+1] == "\n") or
                                            (lines[i] == "\"\"\"\n" and lines[i+1] == "\n")):
                end = i
                break
        if start >= 0 and end >=0:
            temp = []
            if start > 0:
                temp = lines[:start-1]
            temp.extend(lines[end+1:])
            lines = temp
        insert_position = start
    if copyright_exists and not file_modified and not new_license:
        # We do not need to modify the license
        update_hash_in_db(name, new_hash)
        return

    if not copyright_exists and name.endswith(".py") and len(lines) > 0 and lines[0].startswith('#'):
        insert_position = 1
    license = [e + '\n' for e in TEXT.format("Copyright {} (C) Alexey Dynda".format(YEAR)).split('\n')]
    if not copyright_exists:
        license.append("\n")
    if name.endswith(".py"):
        for i in range(len(license)):
            if license[i] != "\n":
                license[i] = "  " + license[i]
        license.insert(0, "\"\"\"\n")
        license.append("\"\"\"\n")
        if not copyright_exists:
            license.append("\n")
    else:
        for i in range(len(license)):
            if license[i] != "\n":
                license[i] = "  " + license[i]
        license.insert(0, "/*\n")
        license.append("*/\n")
        if not copyright_exists:
            license.append("\n")
    for t in reversed(license):
        lines.insert(insert_position,t)

    with open(name, "w") as f:
        for l in lines:
            f.write(l)
    new_hash = str(get_hash(''.join(lines)))
    update_hash_in_db(name, new_hash)
    # print(''.join(lines))


for root, dirs, files in os.walk("."):
    path = root.split(os.sep)
    skip = False
    for d in exclude_paths:
        if root.startswith( d ):
            skip = True
    if not skip:
        for file in files:
            for f in include_files:
                p = root + '/' + file
                if p.endswith( f ):
                    code.append( p )
                    break


TEXT = '''  {}

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
  For further information contact via email on github account.'''

open_hash_db('.hash_db')

for c in code:
    # print(c)
    check_and_update_license(c, False)

save_hash_db('.hash_db')

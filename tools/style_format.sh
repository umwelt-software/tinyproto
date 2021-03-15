#!/bin/sh

find src/ python/ unittest/ examples/ -iname *.h -o -iname *.cpp -o -iname *.inl -o -name *.c | xargs clang-format -i

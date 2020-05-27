#!/bin/sh
cd `dirname $0`/.. || exit 1
if [ $# -gt 0 ]
then
  options="$*"
else
  options="-O0"
fi
if [ -f makefile ]
then
  make clean 1>&2 || exit 1
fi
./configure --coverage --test $options 1>&2 || exit 1
make test 1>&2 || exit 1
exec make -s coverage

#!/bin/sh

cd "`dirname $0`/.."
binary="`basename $0`"
parallel=""

usage () {
cat <<EOF
usage: $binary [ <option> ]

where '<option>' is one of the following

  -h             print this command line option summary
  -1             single configuration option mode (default)
  -2             double configuration options mode
  -3             triple configuration options mode
  -j             build and test in parallel

  --fake         only print and fake building and testing
  --force        force '--coverage' with 'clang'

  --no-coverage  disable coverage
  --no-pedantic  disable pedantic option
EOF
exit 0
}

# All './configure' options except '-p' (pedantic).

all="--default --extreme -m32 --ultimate -c -g -l -s --coverage --profile --compact --no-options --quiet --metrics --stats --no-proofs -fPIC --shared --kitten --no-metrics --no-stats"

tmp=/tmp/m32-support-$$
cat <<EOF > $tmp.c
#include <stdio.h>
int main (void) {
  printf ("%zu\n", sizeof (void*));
  return 0;
}
EOF
m32=yes
if gcc -m32 -o $tmp $tmp.c 1>/dev/null 2>/dev/null
then
  res="`$tmp 2>/dev/null`"
  [ "$res" = 4 ] || m32=no
else
  m32=no
fi
if [ $m32 = no ]
then
  echo "compilation with '-m32' disabled (install 'g++-multilib' first)"
  all="`echo $all|sed -e 's, -m32,,'`"
fi
rm -f $tmp*

# Default mode is to test single configurations ('-1').

mode=1

if [ -t 1 ]
then
  BOLD="\033[1m"
  GREEN="\033[1;32m"
  NORMAL="\033[0m"
  RED="\033[1;31m"
else
  BOLD=""
  GREEN=""
  NORMAL=""
  RED=""
fi

die () {
  echo "${BOLD}$binary: ${RED}error:${NORMAL} $*"
  exit 1
}

fake=no
force=no
coverage=yes
pedantic=yes

while [ $# -gt 0 ]
do
  case "$1" in
    -h) usage;;
    -1) mode=1;;
    -2) mode=2;;
    -3) mode=3;;
    -f) force=yes;;
    --fake) fake=yes;;
    --force) force=yes;;
    --no-coverage) coverage=no;;
    --no-pedantic) pedantic=no;;
    -j|-j*) parallel="$1";;
    *) die "invalid option '$1' (try '-h')";;
  esac
  shift
done

failed () {
  echo
  echo "${BOLD}$binary: ${RED}failed:${NORMAL} $*"
  exit 1
}

count=0

run () {
  configure="`echo ./configure $*|sed -e 's, --default,,'`"
  printf "%-50s" "$configure"
  if [ $fake = no ]
  then
    $configure 1>/dev/null 2>/dev/null
    status=$?
  else
    status=0
  fi
  if [ $status = 0 ]
  then
    printf " && make"
    if [ $fake = no ]
    then
      make 1>/dev/null 2>/dev/null
      status=$?
    else
      status=0
    fi
    if [ $status = 0 ]
    then
      printf " test"
      if [ $fake = no ]
      then
	make test 1>/dev/null 2>/dev/null
	status=$?
      else
	status=0
      fi
      if [ $status = 0 ]
      then
	printf " && make clean"
	if [ $fake = no ]
	then
	  make clean 1>/dev/null 2>/dev/null
	  status=$?
	else
	  status=0
	fi
	if [ $status = 0 ]
	then
	  echo
	  count=`expr $count + 1`
	else
	  failed "$configure && make test && make clean"
	fi
      else
	failed "$configure && make test"
      fi
    else
      failed "$configure && make"
    fi
  else
    failed "$configure"
  fi
}

#----------------------------------------------------#

case $mode in
  1) modestr=single;;
  2) modestr="single and double";;
  3) modestr="single, double and triple";;
esac

echo
echo "testing $modestr combinations"
if [ "$CC" = "" ]
then
  echo "using default compiler 'gcc' (no 'CC' environment variable)"
else
  echo "using '$CC' compiler (from environment variable 'CC')"
  case "$CC" in
    clang*)
      if [ $force = yes ]
      then
        echo "forced to use '--coverage' for 'clang' (due to '-f')"
	coverage=yes
      else
        echo "not using '--coverage' for 'clang' (use '-f' to force)"
	coverage=no
      fi
      ;;
  esac
fi

if [ x"$parallel" = x ]
then
  echo "not forcing parallel build and testing (no '-j' option)"
else
  TMP=""
  for flag in $MAKEFLAGS
  do
    case x"$flag" in
      x-j*) continue;;
    esac
    TMP="$TMP${flag} "
  done
  echo
  MAKEFLAGS="$TMP${parallel}"
  echo "setting 'MAKEFLAGS=$MAKEFLAGS' (due to '$parallel')"
  export MAKEFLAGS
  TISSATFLAGS="${parallel}"
  echo "setting 'TISSATFLAGS=$TISSATFLAGS' (due to '$parallel')"
  export TISSATFLAGS
fi

if [ $coverage = no ]
then
  all=`echo "$all" | sed -e 's,--coverage ,,'`
fi

#----------------------------------------------------#

# single options both pedantic and not pedantic

echo
echo "---- [ single configurations ]" \
"----------------------------------------------"
echo

for p in yes no
do
  [ $pedantic = no -a $p = yes ] && continue
  for first in $all
  do
    [ $first = --no-metrics ] && continue
    options="$first"
    [ $p = yes ] && options="-p $options"
    run $options
  done
done

#----------------------------------------------------#

redundant () {
  case $1$2 in
    -c-g) return 0;;
    --default--no-metrics) return 0;;
    --default--no-stats) return 0;;
    --extreme--ultimate) return 0;;
    --extreme--compact) return 0;;
    --extreme-l) return 0;;
    --extreme--no-options) return 0;;
    --extreme--quiet) return 0;;
    --extreme--no-metrics) return 0;;
    --extreme--no-stats) return 0;;
    -g-l) return 0;;
    -g--metrics) return 0;;
    -g--stats) return 0;;
    -g-s) return 0;;
    --metrics--no-metrics) return 0;;
    --metrics--no-stats) return 0;;
    --metrics--stats) return 0;;
    --no-metrics--no-stats) return 0;;
    --stats--no-stats) return 0;;
    --ultimate--compact) return 0;;
    --ultimate-l) return 0;;
    --ultimate--no-options) return 0;;
    --ultimate--quiet) return 0;;
    --ultimate--no-metrics) return 0;;
    --ultimate--no-stats) return 0;;
    --ultimate--no-proofs) return 0;;
    -l--metrics) return 0;;
    -l--stats) return 0;;
    -l--quiet) return 0;;
  esac
  return 1
}

#----------------------------------------------------#

if [ $mode -gt 1 ]
then

# double options both pedantic and not pedantic

echo
echo "---- [ double configurations ]" \
"----------------------------------------------"
echo

for p in yes no
do
  [ $pedantic = no -a $p = yes ] && continue
  for first in $all
  do
    [ x$first = x--default ] && continue;
    [ x$first = x--no-metrics ] && continue;
    metrics=no
    [ x$first = x-g -o x$first = x-l ] && metrics=yes
    for second in `echo -- $all|fmt -1|sed "1,/$first/d"`
    do
      if redundant $first $second; then continue; fi
      [ x$second = x--no-metrics -a $metrics = no ] && continue
      options="$first $second"
      [ $p = yes ] && options="-p $options"
      run $options
    done
  done
done

fi

#----------------------------------------------------#

if [ $mode -gt 2 ]
then

# triple options both pedantic and not pedantic

echo
echo "---- [ triple configurations ]" \
"----------------------------------------------"
echo

for p in yes no
do
  [ $pedantic = no -a $p = yes ] && continue
  for first in $all
  do
    [ x$first = x--default ] && continue;
    [ x$first = x-g -o x$first = x-l ] && metrics=yes
    for second in `echo -- $all|fmt -1|sed "1,/$first/d"`
    do
      if redundant $first $second; then continue; fi
      metrics=no
      [ x$first = x-g -o x$first = x-l ] && metrics=yes
      [ x$second = x--no-metrics -a $metrics = no ] && continue
      [ x$second = x-g -o x$second = x-l ] && metrics=yes
      for third in `echo -- $all|fmt -1|sed "1,/$second/d"`
      do
	if redundant $first $third; then continue; fi
	if redundant $second $third; then continue; fi
        [ x$third = x--no-metrics -a $metrics = no ] && continue
	options="$first $second $third"
	[ $p = yes ] && options="-p $options"
	run $options
      done
    done
  done
done

fi

#----------------------------------------------------#
echo
echo "---- [ summary ]" \
"------------------------------------------------------------"
echo

echo "All $count $modestr combinations" \
"${GREEN}successfully${NORMAL} tested."

case $modestr in
  single)
    echo
    echo "Consider to run double ('-2') or even triple mode ('-3') too."
    ;;
  double)
    echo
    echo "Consider to run triple mode ('-3') too."
    ;;
esac

#!/bin/sh

if [ -t 2 ]
then
  BOLD="\033[1m"
  GREEN="\033[1;32m"
  NORMAL="\033[0m"
  RED="\033[1;31m"
  YELLOW="\033[1;33m"
else
  BOLD=""
  GREEN=""
  NORMAL=""
  RED=""
  YELLOW=""
fi

script=`basename $0`

die () {
  echo "${BOLD}$script: ${RED}error:${NORMAL} $*" 1>&2
  exit 1
}

fatal () {
  echo "${BOLD}$script: ${RED}fatal error:${NORMAL} $*" 1>&2
  exit 1
}

msg () {
  echo "$script: $*" 1>&2
}

wrn () {
  echo "${BOLD}$script: ${YELLOW}warning:${NORMAL} $*" 1>&2
}

cd `dirname $0`/..
root=`pwd`

tmp=/tmp/`basename $script .sh`
name=$tmp.name
rm -f $tmp*

usage () {
cat <<EOF
usage: $script [-h][-f]
EOF
}

force=no

while [ $# -gt 0 ]
do
  case "$1" in
    -h) usage; exit 0;;
    -f) force=yes;;
    *) die "invalid option '$1'";;
  esac
  shift 
done

if [ $force = yes ]
then
  wrn "not checking commit status (due to '-f')"
  forceoption="-f"
else
  msg "will check commit status (use '-f' to disable)"
  forceoption=""
fi

msg "first generating source code tar file"
./scripts/make-source-release.sh $forceoption -t $name || exit 1
source="`cat $name`"
[ -f "$source" ] || fatal "can not access tar '$source'"
msg "source code tar '$source'"

version=`cat VERSION`
base=kissat-${version}-starexec
dir=/tmp/$base

rm -rf $dir

mkdir $dir
mkdir $dir/bin
mkdir $dir/build
mkdir $dir/archive

msg "generated '$dir' structure"

cp -p "$source" $dir/archive
msg "copied source code archive"

starexec_build=$dir/starexec_build
cat <<EOF >$starexec_build
#!/bin/sh
cd build
exec ./build.sh
EOF
chmod 755 $starexec_build

build=$dir/build/build.sh
cat <<EOF > $build
#!/bin/sh
tar xf ../archive/kissat*
mv kissat* kissat
cd kissat
./configure --competition --test
make all || exit 1
build/tissat || exit 1
exec install -s build/kissat ../../bin/
EOF
chmod 755 $build

starexec_run_default=$dir/bin/starexec_run_default
cat <<EOF >$starexec_run_default
#!/bin/sh
exec ./kissat \$1 \$2/proof.out
EOF
chmod 755 $starexec_run_default

msg "generated build and run scripts"
version=`cat VERSION`

description=$dir/starexec_description.txt
cat <<EOF>$description
Keep it simple bare metal SAT Solver
EOF
msg "included the following description:"
printf $BOLD
cat $description
printf $NORMAL
 
zipfile=/tmp/$base.zip
rm -f $zipfile

cd $dir
zip -q -r $zipfile .
cd /tmp/

msg "generated ${GREEN}'$zipfile'${NORMAL}"
BYTES="`ls --block-size=1 -s $zipfile 2>/dev/null |awk '{print $1}'`"
msg "zip file has $BYTES bytes"

echo $zipfile

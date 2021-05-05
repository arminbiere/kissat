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

msg () {
  echo "$script: $*" 1>&2
}

wrn () {
  echo "${BOLD}$script: ${YELLOW}warning:${NORMAL} $*" 1>&2
}

force=no
tar=""

usage () {
cat <<EOF
usage: $script [-h|--help] [-f|--force][-t <file>]
EOF
exit 0
}

while [ $# -gt 0 ]
do
  case "$1" in
    -h|--help) usage;;
    -f|--force) force=yes;;
    -t)
      shift
      [ $# = 0 ] && die "argument to '-t' missing"
      tar="$1"
      ;;
    *) die "invalid option '$1' (try '-h')";;
  esac
  shift
done

cd "`dirname $0`/.."
[ -d .git ] || die "could not find '.git' directory"
FULLID="`git show 2>/dev/null|awk '{print $2; exit}'`"
[ "$FULLID" = "" ] && die "could not get full git ID ('git show' failed)"
if git diff --quiet
then
  CHANGES=no
else
  CHANGES=yes
fi

if [ $force = yes ]
then
  if [ $CHANGES = yes ]
  then
    wrn "uncommitted changes (but forced to continue)"
  else
    msg "no uncommitted changes (no need to use '-f')"
  fi
else
  if [ $CHANGES = yes ]
  then
    die "uncommitted changes ('git commit' or '-f')"
  else
    msg "no uncommitted changes (as expected)"
  fi
fi

SHORTID="`echo $FULLID|sed -e 's,^\(........\).*,\1,'`"
[ "$SHORTID" = "" ] && die "could not get short git ID"
[ -f VERSION ] || die "could not find 'VERSION' file"
VERSION="`cat VERSION`"
[ "$VERSION" = "" ] && die "invalid 'VERSION' file"
NAME=kissat-$VERSION-$SHORTID
DIR=/tmp/$NAME
ARCHIVE=/tmp/$NAME.tar.xz
LIMIT=10

if false
then
  msg CHANGES $CHANGES
  msg VERSION $VERSION
  msg FULLID $FULLID
  msg SHORTID $SHORTID
  msg NAME $NAME
  msg DIR $DIR
  msg ARCHIVE $ARCHIVE
  msg LIMIT $LIMIT
fi

if [ -d $DIR ]
then
  msg "reusing '$DIR'"
  rm -rf $DIR/*
else
  msg "new directory '$DIR'"
  mkdir $DIR || exit 1
fi

msg "copying complete repository"
git archive HEAD | tar -x -C $DIR

msg "removing redundant files"
rm $DIR/scripts/build-and-test-all-configurations.sh || exit 1
rm $DIR/scripts/make-source-release.sh || exit 1
rm $DIR/scripts/prepare-competition.sh || exit 1
rm $DIR/.gitignore || exit 1
rm $DIR/.vimdir || exit 1
sed -i -e "s,ID=unknown,ID=$FULLID," $DIR/scripts/generate-build-header.sh
msg "removing CNF files with more than ${LIMIT}k bytes"
find $DIR/test/cnf -size +${LIMIT}k -exec rm {} \;
msg "generating archive '$ARCHIVE'"

rm -f $ARCHIVE
cd /tmp
tar cJf $ARCHIVE $NAME || exit 1
msg "generated '${GREEN}$ARCHIVE${NORMAL}'"
BYTES="`ls --block-size=1 -s $ARCHIVE 2>/dev/null |awk '{print $1}'`"
msg "archive has $BYTES bytes"

if [ "$tar" = "" ]
then
  echo "$ARCHIVE"
else
  echo "$ARCHIVE" > "$tar"
  msg "wrote archive name to '$tar'"
fi

exit 0

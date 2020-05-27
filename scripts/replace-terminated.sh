#!/bin/sh
count=`grep 'TERMINATED ([0-9][0-9]*)' *.c|wc -l`
i=1
replace=`expr $count - 1`
echo -n "sed -i "
while [ $i -le $count ]
do
  last="`sed -e '/TERMINATED ([0-9][0-9]*)/!d' -e 's,.*TERMINATED (\([0-9][0-9]*\)).*,\1,' *.c|tail -$i|head -1`"
  echo "-e \'s,TERMINATED (${last}),TERMINATED (${replace}),\'"
  replace=`expr $replace - 1`
  i=`expr $i + 1`
done | xargs echo -n
echo " *.c"

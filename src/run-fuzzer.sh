#!/bin/sh
kissat --range > options || exit 1
i=0
trap "killall runcnfuzz" 2 10 15
CNFUZZOPTIONS=options 
export CNFUZZOPTIONS
while [ $i -lt 12 ]
do
  runcnfuzz -i ../build/kissat --time=3 &
  i=`expr $i + 1`
done
wait

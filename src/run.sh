##!/bin/sh
kitten=`kitten $1|grep '^s'`
kissat=`kissat $1|grep '^s'`
echo "kitten $kitten"
echo "kissat $kissat"
[ "$kitten" = "$kissat" ] && exit 20
exit 1

#!/bin/sh
sed \
  -e '/^$/d' \
  -e '/^File/d' \
  -e s,\',,g \
  -e 's,[:%], ,' | \
awk '
/^Lines/{coverage=$3;lines=$5}
/^Creating/{
    printf "%6.2f %% %-20s %12u lines\n", coverage, $2, lines
}
END{
  printf "%6.2f %% %-20s %12u lines\n", coverage, "TOTAL COVERAGE", lines
}
' | \
grep -v '^100.00 %' | \
sort -r -n | \
awk '
/TOTAL COVERAGE/ {
  print "-------------------------------------------------"
  print $0
  print "-------------------------------------------------"
  next
}
{ print }'

#!/bin/sh

#
# Test de fonctionnalité de date (option -t)
#

TEST=$(basename $0 .sh)     #take the part of program name before the .sh and save in TEST

TMP=/tmp/$TEST
LOG=$TEST.log       #create log file with the TEST name
V=${VALGRIND}		# appeler avec la var. VALGRIND à "" ou "valgrind -q"

exec 2> $LOG        #write standard errors in LOG file
set -x      # turn on debug

rm -f *.tmp         #remove all *.tmp files; if files doesn't exist you don't get error message

fail ()
{
    echo "==> Échec du test '$TEST' sur '$1'."
    echo "==> Log : '$LOG'."
    echo "==> Exit"
    exit 1
}

for i in $(seq 1 10)        #for i between 1 and 10
do
    date +%m:%Y
done > f1.tmp

$V ./detecter -t %m:%Y -i1 -l10 cat /dev/null > f2.tmp \
	&& cmp -s f1.tmp f2.tmp 		|| fail "date"  #if cmp return non-zero integer, then call function fail
# \ in line 31 means execute line 31 and 32 as one command(i.e. in one line)

exit 0

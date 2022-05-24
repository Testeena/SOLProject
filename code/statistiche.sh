if [ $# = 0 ]
  then
    echo "Usage: ./statistiche.sh pathToLogfile"
    exit 1
fi

echo -e "-------------------------- LOGFILE STATS --------------------------"

echo -e -n "Number of reads:           "
grep "READ" $1 | wc -l

echo -e -n "Average Bytes Read:        "
grep "Bytes Read" $1 | cut -d " " -f 3 | awk '{SUM += $1; COUNT += 1} END {if(COUNT > 0){print int(SUM/COUNT)}else{print "0"}}'

echo -e -n "Number of writes:          "
grep "WRITE" $1 | wc -l

echo -e -n "Average Bytes Written:     "
grep "Bytes Written" $1 | cut -d " " -f 3 | awk '{SUM += $1; COUNT += 1} END {if(COUNT > 0){print int(SUM/COUNT)}else{print "0"}}'

echo -e -n "Number of open:            "
grep "OPEN" $1 | wc -l

echo -e -n "Number of closes:          "
grep "CLOSE" $1 | wc -l

echo -e -n "Number of locks:           "
grep "LOCK" $1 | wc -l

echo -e -n "Number of unlocks:         "
grep "UNLOCK" $1 | wc -l
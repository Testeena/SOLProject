#!/bin/bash

bin/server configs/config2.txt &
serverpid=$!

/bin/sleep 2

# max bytes capacity reached eviction test + saving evicted files
bin/client -p -t 200 -f solsock.sk -W files/files3/file12,files/files3/file13

bin/client -p -t 200 -f solsock.sk -W files/files3/file14 -D test2savedfiles/evicted

echo -e "\n-------------- file12 and file13 are in test2savedfiles/evicted1 directory ---------------\n"

# max files capacity reached eviction test + throwing away evicted files
bin/client -p -t 200 -f solsock.sk -w files/files1

bin/client -p -t 200 -f solsock.sk -W files/files2/file6.txt,files/files2/file7.txt,files/files2/file8.txt,files/files2/file9.txt,files/files2/file10.txt

bin/client -p -t 200 -f solsock.sk -W files/files2/file11.txt

echo -e "\n------------------------------- file1.txt is thrown away ---------------------------------\n"

bin/client -p -t 200 -f solsock.sk -w files/files3,0 -D test2savedfiles/evicted2

/bin/sleep 5

kill -s SIGHUP $serverpid
wait $serverpid

exit 0
#!/bin/bash

bin/server configs/config2.txt &
serverpid=$!

/bin/sleep 0.5

# max bytes capacity reached eviction test + saving evicted files
bin/client -p -f solsock.sk -W files/files3/file12.txt,files/files3/file13.txt

bin/client -p -f solsock.sk -W files/files3/file14.txt -D test2savedfiles/evicted

# max files capacity reached eviction test + throwing away evicted files
bin/client -p -f solsock.sk -w files/files1

bin/client -p -f solsock.sk -W files/files2/file6.txt,files/files2/file7.txt,files/files2/file8.txt,files/files2/file9.txt,files/files2/file10.txt

bin/client -p -f solsock.sk -W files/files2/file11.txt

bin/client -p -f solsock.sk -w files/files3 -D test2savedfiles/evicted2

kill -s SIGHUP $serverpid
wait $serverpid

exit 0
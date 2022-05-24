#!/bin/bash

# valgrind --leak-check=full
valgrind --leak-check=full bin/server configs/config1.txt &
serverpid=$!

/bin/sleep 2

bin/client -p -t 200 -f solsock.sk -W files/files1/file1.txt,files/files1/file2.txt -r files/files1/file1.txt,files/files1/file2.txt -d test1savedfiles/1

bin/client -p -t 200 -f solsock.sk -w files/files2 -R 0 -d test1savedfiles/2

bin/client -p -t 200 -f solsock.sk -l files/files1/file1.txt -c files/files1/file1.txt

bin/client -p -t 200 -f solsock.sk -l files/files1/file2.txt -r files/files1/file2.txt -u files/files1/file2.txt

bin/client -p -t 200 -f solsock.sk -c files/files2/file9.txt

kill -s SIGHUP $serverpid
wait $serverpid

exit 0
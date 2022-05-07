#!/bin/bash

PATH="$(pwd -P)"

bin/server configs/config3.txt &
SERVERPID=$!
/bin/sleep 10 && kill -2 ${SERVERPID} &

clientpids=()
for i in {1..10}; do
    /bin/bash ./scripts/test3clients.sh &
    clientpids+=($!)
    /bin/sleep 0.1
done

for i in "${clientpids[@]}"; do
    kill -9 ${i}
    wait ${i}
done

wait ${SERVERPID}

exit 0
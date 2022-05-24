#!/bin/bash

bin/server configs/config3.txt &
server=$!

/bin/sleep 2

/bin/sleep 30 && kill -2 ${server} &

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

wait ${server}

exit 0
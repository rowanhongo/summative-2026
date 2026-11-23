#!/bin/bash
# runs every demo scenario against one server, use: bash test_demo.sh > output.txt 2>&1
cd "$(dirname "$0")"
PORT=8080
mkdir -p tmp

./server $PORT > tmp/server_log.txt 2>&1 &
SERVER=$!
sleep 1

echo "##### 1. Successful authentication, event list and registration (S1001, event 101)"
printf 'S1001\n101\n0\n' | ./client 127.0.0.1 $PORT

echo
echo "##### 2. Unsuccessful authentication (S9999 is not a registered student)"
printf 'S9999\n' | ./client 127.0.0.1 $PORT

echo
echo "##### 3. Same student registers for the same event twice (S1002, event 101)"
printf 'S1002\n101\n101\n0\n' | ./client 127.0.0.1 $PORT

echo
echo "##### 4. Event that does not exist (S1003, event 999)"
printf 'S1003\n999\n0\n' | ./client 127.0.0.1 $PORT

echo
echo "##### 5. Raw connection that never logs in tries to see events and register"
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'LIST\nREGISTER 101\n' >&3
for i in 1 2
do
    read -t 2 -u 3 reply
    echo "$reply"
done
exec 3>&-

echo
echo "##### 6. Eight clients connected at the same time, all ask for event 102 (capacity 4) together"
echo "(each client logs in, waits 3 seconds and then sends its request, so the requests arrive at the same moment)"
pids=""
for id in S1001 S1002 S1003 S1004 S1005 S1006 S1007 S1008
do
    (printf "$id\n"; sleep 3; printf "102\n"; sleep 1; printf "0\n") | ./client 127.0.0.1 $PORT > tmp/race_$id.txt &
    pids="$pids $!"
done
wait $pids

for id in S1001 S1002 S1003 S1004 S1005 S1006 S1007 S1008
do
    echo "--- client $id (server replies only) ---"
    grep -E "^(OK|ERROR)" tmp/race_$id.txt
done

echo
echo "##### 7. Event 102 is full now, another student tries it (S1009)"
printf 'S1009\n102\n0\n' | ./client 127.0.0.1 $PORT

echo
echo "##### 8. A client is killed in the middle of a session (S1010)"
(printf 'S1010\n'; sleep 5) | ./client 127.0.0.1 $PORT > tmp/lost.txt &
CLIENT=$!
sleep 1
kill -9 $CLIENT
sleep 1
echo "client process $CLIENT killed with kill -9, the server log below shows how it was handled"

echo
echo "##### 9. Server shutdown with ctrl+c (SIGINT), then the whole server log"
kill -INT $SERVER
wait $SERVER
echo
cat tmp/server_log.txt

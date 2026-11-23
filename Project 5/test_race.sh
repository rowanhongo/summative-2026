#!/bin/bash
# repeats the last place race 20 times, use: bash test_race.sh > race_output.txt 2>&1
# every round starts a new server and 8 clients ask for event 102 (capacity 4) at the same moment
cd "$(dirname "$0")"
PORT=8081
mkdir -p tmp
good=0

for round in $(seq 1 20)
do
    ./server $PORT > tmp/round.txt 2>&1 &
    SERVER=$!
    sleep 0.5

    # all clients log in first and send their request after 2 seconds, together
    pids=""
    for id in S1001 S1002 S1003 S1004 S1005 S1006 S1007 S1008
    do
        (printf "$id\n"; sleep 2; printf "102\n"; sleep 0.5; printf "0\n") | ./client 127.0.0.1 $PORT > tmp/round_$id.txt &
        pids="$pids $!"
    done

    # the server is stopped only after every client is done
    wait $pids
    kill -INT $SERVER
    wait $SERVER

    ok=$(cat tmp/round_S100*.txt | grep -c "^OK Registered")
    full=$(cat tmp/round_S100*.txt | grep -c "^ERROR Event is full")
    echo "round $round: $ok registered, $full told the event is full | $(grep '^  102' tmp/round.txt)"

    if [ $ok -eq 4 ] && [ $full -eq 4 ]; then
        good=$((good + 1))
    else
        # shows which client got no answer and what it printed
        for f in $(grep -L -E "^(OK Registered|ERROR Event is full)" tmp/round_S100*.txt)
        do
            echo "  no answer for $f:"
            cat $f
        done
    fi
done

echo
echo "rounds where exactly 4 registered and 4 were refused: $good of 20"

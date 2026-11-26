# Project 5: TCP University Event Registration System

A server and a client for event registration. Students connect to the server, log in with their student id, see the events and register for one. Many clients can be connected at the same time. The server uses one POSIX thread for each client.

## Files

- server.c: the server.
- client.c: the client.
- test_demo.sh: runs every scenario in the brief against one server.
- output.txt: the saved output of test_demo.sh.
- test_race.sh: repeats the last place race 20 times.
- race_output.txt: the saved output of test_race.sh.

## Environment

Ubuntu 24.04 on WSL2 and GCC 13.3.0.

## Compile

```
gcc -Wall -pthread -o server server.c
gcc -Wall -o client client.c
```

## Run

Start the server in one terminal. The port is optional and the default is 8080.

```
./server 8080
```

Start a client in another terminal. The host and the port are optional. The defaults are 127.0.0.1 and 8080.

```
./client 127.0.0.1 8080
```

Press Ctrl+C in the server terminal to shut it down. It tells the connected clients, prints the final registrations and stops.

## Input

The client asks for a student id and then for an event id. Type 0 to leave.

Registered students: S1001, S1002, S1003, S1004, S1005, S1006, S1007, S1008, S1009 and S1010.

| Event id | Name | Capacity |
| --- | --- | --- |
| 101 | AI Workshop | 5 |
| 102 | Cybersecurity Seminar | 4 |
| 103 | Research Conference | 6 |

## Example session

```
Enter your student ID: S1001
OK Welcome S1001

Available events:
ID    Name                      Registered
101   AI Workshop               0/5
102   Cybersecurity Seminar     0/4
103   Research Conference       0/6

Enter the event ID to register for (0 to exit): 101
OK Registered for AI Workshop (1/5 places taken)

Available events:
ID    Name                      Registered
101   AI Workshop               1/5
102   Cybersecurity Seminar     0/4
103   Research Conference       0/6

Enter the event ID to register for (0 to exit): 0
OK Goodbye
Registration session closed. Goodbye, S1001.
```

## The messages between client and server

The client and the server send text lines over TCP. Each line ends with a line feed.

| Client sends | Server answers |
| --- | --- |
| LOGIN id | OK Welcome id and then the event list, or ERROR Unknown student ID and the connection is closed |
| LIST | the event list |
| REGISTER eventId | OK Registered for name (count/capacity places taken) or one of the errors below, and then the event list |
| QUIT | OK Goodbye |

The event list is one line per event, EVENT id capacity count name, and a last line END.

| Error | When |
| --- | --- |
| ERROR Please log in first | any command except LOGIN before a successful login |
| ERROR No such event | the event id does not exist |
| ERROR You are already registered for this event | the student already has a place in that event |
| ERROR Event is full | the event has reached its capacity |
| ERROR Already logged in | a second LOGIN on the same connection |
| ERROR Unknown command | anything else |
| ERROR Server is full | 20 clients are already connected |

## How the server works

- main() creates the socket, calls bind(), listen() and accept(). For each client it starts a thread with pthread_create() and detaches it.
- One mutex protects the events and the list of connected clients.
- registerStudent() checks the event, checks for a duplicate and checks the capacity, and then updates the count. All of it happens while the mutex is held. Two students can never take the same last place.
- The list of connected clients is printed every time a client connects or leaves.
- When a client closes or crashes, fgets() on its socket returns NULL. The thread removes the client from the list and ends. Registrations made before the disconnect are kept.
- A client that sends nothing for 60 seconds is dropped.
- The server ignores SIGPIPE, so writing to a client that has left does not stop the server.

## Tests

```
bash test_demo.sh > output.txt 2>&1
bash test_race.sh > race_output.txt 2>&1
```

test_demo.sh starts a server and runs these scenarios. The client output and the server log are in output.txt.

| Scenario | What it shows |
| --- | --- |
| 1 | S1001 logs in, sees the events and registers for 101 |
| 2 | S9999 is refused |
| 3 | S1002 registers for 101 and then tries 101 again, which is refused as a duplicate |
| 4 | S1003 asks for event 999, which does not exist |
| 5 | a raw connection that never logs in is refused for LIST and REGISTER |
| 6 | eight clients are connected at the same time and ask for event 102 (capacity 4) together. Four get a place and four are told the event is full |
| 7 | S1009 asks for event 102 after it is full and is refused |
| 8 | a client is killed with kill -9 and the server logs the lost connection |
| 9 | Ctrl+C shuts the server down and it prints the final registrations |

The end of the server log in output.txt:

```
[Server] S1010 logged in
[Server] Lost connection to S1010
[Server] Connected clients (0):

[Server] Shutting down
[Server] Final registrations:
  101 AI Workshop 2/5: S1001 S1002
  102 Cybersecurity Seminar 4/4: S1001 S1002 S1003 S1004
  103 Research Conference 0/6:
```

test_race.sh starts a new server 20 times. Each time 8 clients ask for event 102 at the same moment. The last line of race_output.txt is:

```
rounds where exactly 4 registered and 4 were refused: 20 of 20
```

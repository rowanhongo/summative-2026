# Project 4: Multithreaded Library Book Reservation System

reservations.c simulates a library with three threads. A student thread submits reservation requests. A staff thread processes them. A monitor thread prints the statistics. The threads share a queue that holds 5 requests, and the program stops after 15 reservations.

## Files

- reservations.c: the source code.
- output.txt: the saved output of one run.

## Environment

Ubuntu 24.04 on WSL2 and GCC 13.3.0.

## Compile

```
gcc -Wall -pthread -o reservations reservations.c
```

## Run

```
./reservations
```

There is no input. A run takes about 52 seconds, because the staff thread needs 3 seconds for each of the 15 reservations.

## The threads

| Thread | Function | What it does |
| --- | --- | --- |
| Student (producer) | producer() | Takes 1 second to make a request. Gives it the next reservation id (1 to 15) and a book id from 1 to 20. Puts it in the queue. Waits when the queue is full. |
| Staff (consumer) | consumer() | Takes a request from the queue. Spends 3 seconds on it. Approves it if the book id is even and rejects it if the book id is odd. Waits when the queue is empty. |
| Monitor | monitor() | Every 4 seconds prints the submitted, processed, approved and rejected totals and the queue size. Stops after the last reservation is processed. |

## Synchronisation

- One mutex (lock) protects the queue and all the counters. Every thread locks it before it reads or changes them.
- The queue is a circular buffer. head, tail and count are updated only while the lock is held.
- notFull is the condition variable the student thread waits on when count is 5. The staff thread signals it after it takes a request.
- notEmpty is the condition variable the staff thread waits on when count is 0. The student thread signals it after it adds a request.
- The waits are in while loops, so a thread checks the condition again when it wakes up.
- The 1 second and 3 second waits happen outside the lock, so the other threads are not blocked while one thread is busy.
- The monitor takes the lock before it prints, so the numbers it shows belong together.
- The student and staff threads each stop after 15 reservations. The monitor stops when processed is 15. main() waits for all three with pthread_join() and then destroys the mutex and the condition variables.

## Expected output

Every line starts with the seconds since the program started. The start of a run:

```
[ 0s] [Staff] Queue empty, waiting
[ 1s] [Student] Submitted reservation 1 for book 4 (queue size 1)
[ 1s] [Staff] Processing reservation 1 for book 4
[ 2s] [Student] Submitted reservation 2 for book 7 (queue size 1)
[ 3s] [Student] Submitted reservation 3 for book 18 (queue size 2)
[ 4s] [Monitor] submitted 3, processed 0, approved 0, rejected 0, queue size 2
[ 4s] [Staff] Reservation 1 approved
[ 4s] [Staff] Processing reservation 2 for book 7
```

The queue gets full and the student thread waits:

```
[ 8s] [Student] Submitted reservation 8 for book 13 (queue size 5)
[ 9s] [Student] Queue full, reservation 9 is waiting
[10s] [Staff] Reservation 3 approved
[10s] [Staff] Processing reservation 4 for book 16
[10s] [Student] Submitted reservation 9 for book 10 (queue size 5)
```

The end of the run:

```
[51s] [Staff] Reservation 15 approved
[52s] [Monitor] submitted 15, processed 15, approved 10, rejected 5, queue size 0
[52s] All 15 reservations processed, program finished
```

## Notes

- The book ids come from rand() and the program does not seed it, so every run gives the same book ids. The order of lines that happen in the same second can change.
- The queue size never goes above 5, and approved plus rejected is always equal to processed.

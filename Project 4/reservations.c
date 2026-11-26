#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 5
#define TOTAL 15

struct request
{
    int id;
    int bookId;
};

// everything below is shared between the threads
// it is only read or changed while holding lock
struct request queue[SIZE];
int head = 0;
int tail = 0;
int count = 0;
int submitted = 0;
int processed = 0;
int approved = 0;
int rejected = 0;

pthread_mutex_t lock;
pthread_cond_t notFull;
pthread_cond_t notEmpty;

long startTime;

// monotonic clock, the normal clock can jump and the times would be wrong
int elapsed()
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec - startTime;
}

void *producer(void *arg)
{
    struct request r;
    int i;

    for (i = 1; i <= TOTAL; i++)
    {
        // preparing a request takes 1 second, done without the lock
        sleep(1);
        r.id = i;
        r.bookId = rand() % 20 + 1;

        pthread_mutex_lock(&lock);

        // while and not if, because a woken thread has to check again
        // wait releases the lock, so the consumer can still take requests
        while (count == SIZE)
        {
            printf("[%2ds] [Student] Queue full, reservation %d is waiting\n", elapsed(), r.id);
            pthread_cond_wait(&notFull, &lock);
        }

        queue[tail] = r;
        tail = (tail + 1) % SIZE;
        count++;
        submitted++;
        printf("[%2ds] [Student] Submitted reservation %d for book %d (queue size %d)\n", elapsed(), r.id, r.bookId, count);

        // tell the consumer there is something to take
        pthread_cond_signal(&notEmpty);
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

void *consumer(void *arg)
{
    struct request r;
    int i;

    for (i = 0; i < TOTAL; i++)
    {
        pthread_mutex_lock(&lock);

        // nothing is removed from an empty queue
        while (count == 0)
        {
            printf("[%2ds] [Staff] Queue empty, waiting\n", elapsed());
            pthread_cond_wait(&notEmpty, &lock);
        }

        r = queue[head];
        head = (head + 1) % SIZE;
        count--;

        // a place is free now, so wake the producer
        pthread_cond_signal(&notFull);
        pthread_mutex_unlock(&lock);

        printf("[%2ds] [Staff] Processing reservation %d for book %d\n", elapsed(), r.id, r.bookId);

        // the 3 seconds of work happen outside the lock so submitting is not blocked
        sleep(3);

        pthread_mutex_lock(&lock);
        processed++;

        // even book ids are available, odd ones are not
        if (r.bookId % 2 == 0)
        {
            approved++;
            printf("[%2ds] [Staff] Reservation %d approved\n", elapsed(), r.id);
        }
        else
        {
            rejected++;
            printf("[%2ds] [Staff] Reservation %d rejected, book %d is not available\n", elapsed(), r.id, r.bookId);
        }

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

void *monitor(void *arg)
{
    int done = 0;

    while (!done)
    {
        sleep(4);

        // the lock gives a consistent snapshot of all the counters
        pthread_mutex_lock(&lock);
        printf("[%2ds] [Monitor] submitted %d, processed %d, approved %d, rejected %d, queue size %d\n",
               elapsed(), submitted, processed, approved, rejected, count);

        // stops once every reservation has been processed
        if (processed == TOTAL)
            done = 1;

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main()
{
    pthread_t student, staff, watcher;
    struct timespec t;

    setbuf(stdout, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t);
    startTime = t.tv_sec;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&notFull, NULL);
    pthread_cond_init(&notEmpty, NULL);

    pthread_create(&student, NULL, producer, NULL);
    pthread_create(&staff, NULL, consumer, NULL);
    pthread_create(&watcher, NULL, monitor, NULL);

    // each thread ends by itself after the 15 reservations, join waits for that
    pthread_join(student, NULL);
    pthread_join(staff, NULL);
    pthread_join(watcher, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notFull);
    pthread_cond_destroy(&notEmpty);

    printf("[%2ds] All %d reservations processed, program finished\n", elapsed(), TOTAL);

    return 0;
}

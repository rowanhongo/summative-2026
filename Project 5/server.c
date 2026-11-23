#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define MAX_CLIENTS 20
#define ID_LEN 16

struct event
{
    int id;
    char name[40];
    int max;
    int count;
    char members[10][ID_LEN];
};

struct client
{
    int fd;
    char id[ID_LEN];
};

struct event events[3] = {
    {101, "AI Workshop", 5, 0},
    {102, "Cybersecurity Seminar", 4, 0},
    {103, "Research Conference", 6, 0}};

char students[10][ID_LEN] = {"S1001", "S1002", "S1003", "S1004", "S1005",
                             "S1006", "S1007", "S1008", "S1009", "S1010"};

// events, clients and numClients are shared by all the client threads
// they are only used while holding lock
struct client clients[MAX_CLIENTS];
int numClients = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t stopFlag = 0;

void handler(int sig)
{
    stopFlag = 1;
}

int isStudent(char id[])
{
    int i;

    for (i = 0; i < 10; i++)
    {
        if (strcmp(students[i], id) == 0)
            return 1;
    }

    return 0;
}

int alreadyIn(struct event *e, char id[])
{
    int i;

    for (i = 0; i < e->count; i++)
    {
        if (strcmp(e->members[i], id) == 0)
            return 1;
    }

    return 0;
}

// the caller must hold the lock
// the line is built first and printed with one printf so other logs cannot cut into it
void printClients()
{
    char list[400] = "";
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd != -1)
        {
            strcat(list, " ");
            strcat(list, clients[i].id);
        }
    }

    printf("[Server] Connected clients (%d):%s\n", numClients, list);
}

// returns the slot of the new client or -1 if the list is full
int addClient(int fd)
{
    int i, slot = -1;

    pthread_mutex_lock(&lock);

    for (i = 0; i < MAX_CLIENTS && slot == -1; i++)
    {
        if (clients[i].fd == -1)
        {
            clients[i].fd = fd;
            strcpy(clients[i].id, "-");
            numClients++;
            slot = i;
        }
    }

    if (slot != -1)
        printClients();

    pthread_mutex_unlock(&lock);

    return slot;
}

void removeClient(int slot)
{
    pthread_mutex_lock(&lock);

    clients[slot].fd = -1;
    numClients--;
    printClients();

    pthread_mutex_unlock(&lock);
}

// the counts are read under the lock so the list is never half updated
void sendEvents(int fd)
{
    char buf[600];
    int len = 0;
    int i;

    pthread_mutex_lock(&lock);

    for (i = 0; i < 3; i++)
    {
        len += snprintf(buf + len, sizeof(buf) - len, "EVENT %d %d %d %s\n",
                        events[i].id, events[i].max, events[i].count, events[i].name);
    }

    pthread_mutex_unlock(&lock);

    snprintf(buf + len, sizeof(buf) - len, "END\n");
    dprintf(fd, "%s", buf);
}

// the duplicate check, the capacity check and the update all happen under one lock
// so two students can never take the same last place
void registerStudent(int fd, char id[], int eventId)
{
    struct event *e = NULL;
    char reply[100];
    int i;

    pthread_mutex_lock(&lock);

    for (i = 0; i < 3; i++)
    {
        if (events[i].id == eventId)
            e = &events[i];
    }

    if (e == NULL)
        strcpy(reply, "ERROR No such event");
    else if (alreadyIn(e, id))
        strcpy(reply, "ERROR You are already registered for this event");
    else if (e->count >= e->max)
        strcpy(reply, "ERROR Event is full");
    else
    {
        strcpy(e->members[e->count], id);
        e->count++;
        snprintf(reply, sizeof(reply), "OK Registered for %s (%d/%d places taken)", e->name, e->count, e->max);
    }

    // logged inside the lock so the log keeps the real order
    printf("[Server] %s asked for event %d: %s\n", id, eventId, reply);

    pthread_mutex_unlock(&lock);

    dprintf(fd, "%s\n", reply);
}

void *handleClient(void *arg)
{
    int fd = *(int *)arg;
    int slot, n;
    int loggedIn = 0, quit = 0;
    char line[100], cmd[16], param[16];
    char id[ID_LEN] = "";
    FILE *in;
    sigset_t set;
    struct timeval tv;

    free(arg);

    // ctrl+c should only be handled by the main thread
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // a client that sends nothing for 60 seconds is dropped
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    slot = addClient(fd);

    if (slot == -1)
    {
        dprintf(fd, "ERROR Server is full\n");
        close(fd);
        return NULL;
    }

    in = fdopen(fd, "r");

    // fgets returns NULL when the client closes or crashes, which ends the loop
    while (fgets(line, sizeof(line), in) != NULL)
    {
        cmd[0] = 0;
        param[0] = 0;
        n = sscanf(line, "%15s %15s", cmd, param);

        if (n < 1)
            continue;

        if (strcmp(cmd, "LOGIN") == 0)
        {
            if (loggedIn)
                dprintf(fd, "ERROR Already logged in\n");
            else if (isStudent(param))
            {
                loggedIn = 1;
                strcpy(id, param);

                pthread_mutex_lock(&lock);
                strcpy(clients[slot].id, id);
                pthread_mutex_unlock(&lock);

                printf("[Server] %s logged in\n", id);
                dprintf(fd, "OK Welcome %s\n", id);
                sendEvents(fd);
            }
            else
            {
                printf("[Server] Login refused for ID '%s'\n", param);
                dprintf(fd, "ERROR Unknown student ID\n");
                break;
            }
        }
        else if (!loggedIn)
        {
            // nothing else is allowed before logging in
            printf("[Server] Refused '%s' from a client that is not logged in\n", cmd);
            dprintf(fd, "ERROR Please log in first\n");
        }
        else if (strcmp(cmd, "LIST") == 0)
            sendEvents(fd);
        else if (strcmp(cmd, "REGISTER") == 0)
        {
            registerStudent(fd, id, atoi(param));
            sendEvents(fd);
        }
        else if (strcmp(cmd, "QUIT") == 0)
        {
            dprintf(fd, "OK Goodbye\n");
            quit = 1;
            break;
        }
        else
            dprintf(fd, "ERROR Unknown command\n");
    }

    if (loggedIn && quit)
        printf("[Server] %s logged out\n", id);
    else if (loggedIn)
        printf("[Server] Lost connection to %s\n", id);

    // registrations made before the disconnect are kept
    removeClient(slot);
    fclose(in);

    return NULL;
}

int main(int argc, char *argv[])
{
    int port = 8080;
    int serverFd, clientFd, i, j;
    int opt = 1;
    int *arg;
    struct sockaddr_in addr;
    struct sigaction sa;
    pthread_t tid;

    if (argc > 1)
        port = atoi(argv[1]);

    setbuf(stdout, NULL);

    // writing to a client that already left must not kill the server
    signal(SIGPIPE, SIG_IGN);

    // no SA_RESTART so accept returns when ctrl+c is pressed
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    for (i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd < 0)
    {
        perror("socket");
        return 1;
    }

    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(serverFd, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("[Server] Listening on port %d\n", port);

    while (!stopFlag)
    {
        clientFd = accept(serverFd, NULL, NULL);

        if (clientFd < 0)
            continue;

        // each client gets its own thread, the fd is passed on the heap
        arg = malloc(sizeof(int));
        *arg = clientFd;
        pthread_create(&tid, NULL, handleClient, arg);
        pthread_detach(tid);
    }

    printf("\n[Server] Shutting down\n");
    close(serverFd);

    // tell the clients that are still connected, their threads then clean up
    pthread_mutex_lock(&lock);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd != -1)
        {
            dprintf(clients[i].fd, "ERROR Server is shutting down\n");
            shutdown(clients[i].fd, SHUT_RDWR);
        }
    }

    printf("[Server] Final registrations:\n");

    for (i = 0; i < 3; i++)
    {
        printf("  %d %s %d/%d:", events[i].id, events[i].name, events[i].count, events[i].max);

        for (j = 0; j < events[i].count; j++)
            printf(" %s", events[i].members[j]);

        printf("\n");
    }

    pthread_mutex_unlock(&lock);

    return 0;
}

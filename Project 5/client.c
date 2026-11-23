#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// shows the prompt and reads a line, returns 0 if the input has ended
int readInput(char prompt[], char buf[], int size)
{
    printf("%s", prompt);

    if (fgets(buf, size, stdin) == NULL)
        return 0;

    buf[strcspn(buf, "\n")] = 0;

    // piped input is not echoed by the terminal so print it here
    if (!isatty(0))
        printf("%s\n", buf);

    return 1;
}

void serverGone()
{
    printf("\nThe server closed the connection.\n");
    exit(1);
}

// reads the event lines from the server until END and prints them as a table
void showEvents(FILE *in)
{
    char line[200], name[60];
    int id, max, count;

    printf("\nAvailable events:\n");
    printf("ID    Name                      Registered\n");

    while (fgets(line, sizeof(line), in) != NULL)
    {
        if (strncmp(line, "END", 3) == 0)
            return;

        if (sscanf(line, "EVENT %d %d %d %59[^\n]", &id, &max, &count, name) == 4)
        {
            printf("%-5d %-25s %d/%d", id, name, count, max);

            if (count >= max)
                printf(" (full)");

            printf("\n");
        }
    }

    serverGone();
}

int main(int argc, char *argv[])
{
    char *host = "127.0.0.1";
    int port = 8080;
    int fd, choice;
    char id[20], line[200], input[20];
    struct sockaddr_in addr;
    FILE *in;

    if (argc > 1)
        host = argv[1];
    if (argc > 2)
        port = atoi(argv[2]);

    // writing to a server that is gone must not kill the client
    signal(SIGPIPE, SIG_IGN);

    fd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
    {
        printf("Invalid server address\n");
        return 1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Could not connect to %s:%d\n", host, port);
        return 1;
    }

    in = fdopen(fd, "r");

    if (!readInput("Enter your student ID: ", id, sizeof(id)))
        return 1;

    dprintf(fd, "LOGIN %s\n", id);

    if (fgets(line, sizeof(line), in) == NULL)
        serverGone();

    printf("%s", line);

    if (strncmp(line, "OK", 2) != 0)
    {
        printf("Authentication failed.\n");
        return 1;
    }

    // the server sends the events right after a successful login
    showEvents(in);

    while (readInput("\nEnter the event ID to register for (0 to exit): ", input, sizeof(input)))
    {
        if (sscanf(input, "%d", &choice) != 1)
        {
            printf("Please enter a number.\n");
            continue;
        }

        if (choice == 0)
            break;

        dprintf(fd, "REGISTER %d\n", choice);

        if (fgets(line, sizeof(line), in) == NULL)
            serverGone();

        printf("%s", line);
        showEvents(in);
    }

    dprintf(fd, "QUIT\n");

    if (fgets(line, sizeof(line), in) != NULL)
        printf("%s", line);

    fclose(in);
    printf("Registration session closed. Goodbye, %s.\n", id);

    return 0;
}

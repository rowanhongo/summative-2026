#include <stdio.h>
#include <stdlib.h>

struct transaction
{
    int id;
    int amount;
    char type;
};

int amounts[6] = {500, 200, 1200, 350, 75, 125};
int totalDeposits = 0;
int totalWithdrawals = 0;

// every second transaction is a withdrawal, the rest are deposits
void fillTransactions(struct transaction *list, int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        list[i].id = 1001 + i;
        list[i].amount = amounts[i];

        if (i % 2 == 0)
            list[i].type = 'D';
        else
            list[i].type = 'W';
    }
}

void processTransactions(struct transaction *list, int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        if (list[i].type == 'D')
        {
            totalDeposits += list[i].amount;
            printf("Transaction %d: deposit of %d\n", list[i].id, list[i].amount);
        }
        else
        {
            totalWithdrawals += list[i].amount;
            printf("Transaction %d: withdrawal of %d\n", list[i].id, list[i].amount);
        }
    }
}

void printSummary()
{
    printf("Total deposits: %d\n", totalDeposits);
    printf("Total withdrawals: %d\n", totalWithdrawals);
    printf("Final balance: %d\n", totalDeposits - totalWithdrawals);
}

int main()
{
    struct transaction *list;
    int n = 6;

    list = malloc(n * sizeof(struct transaction));

    if (list == NULL)
    {
        printf("Memory allocation failed\n");
        return 1;
    }

    fillTransactions(list, n);
    processTransactions(list, n);
    printSummary();

    free(list);

    return 0;
}

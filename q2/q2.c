#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "../colors.h"

void *threadFunc(void *arg)
{
}

struct pizza
{
    int t;
    int specIngr;
    int *ingr;
};

struct chef
{
    int entry;
    int exit;
};

struct customer
{
    int entry;
    int pizzas;
    int *pizzaIDs;
};

struct pizza *pizzaInfo;
struct chef *chefInfo;
struct customer *customerInfo;
int *ingrAmt;

int main()
{
    int chefs, pizzaVars, limIngs, customers, ovens, time2ReachPickup;
    scanf("%d %d %d %d %d %d", &chefs, &pizzaVars, &limIngs, &customers, &ovens, &time2ReachPickup);
    pizzaInfo = (struct pizza *)malloc(sizeof(struct pizza) * pizzaVars);
    chefInfo = (struct chef *)malloc(sizeof(struct chef) * chefs);
    customerInfo = (struct customer *)malloc(sizeof(struct customer) * customers);
    ingrAmt = (int *)malloc(sizeof(int) * limIngs);

    for (int i = 0; i < pizzaVars; i++)
    {
        int pizzaID, pizzaTime, specIng;

        scanf("%d %d %d", &pizzaID, &pizzaTime, &specIng);
        pizzaInfo[pizzaID - 1].t = pizzaTime;
        pizzaInfo[pizzaID - 1].specIngr = specIng;

        pizzaInfo[pizzaID - 1].ingr = (int *)malloc(sizeof(int) * specIng);
        for (int j = 0; j < specIng; j++)
        {
            scanf("%d", &pizzaInfo[pizzaID - 1].ingr[j]);
        }
    }

    for (int i = 0; i < limIngs; i++)
    {
        scanf("%d", &ingrAmt[i]);
    }

    for (int i = 0; i < chefs; i++)
    {
        int entry, exit;
        scanf("%d %d", &entry, &exit);
        chefInfo[i].entry = entry;
        chefInfo[i].exit = exit;
    }

    for (int i = 0; i < customers; i++)
    {
        // entry time, number of pizzas and then IDS of pizzas
        int entry, pizzas;
        scanf("%d %d", &entry, &pizzas);
        customerInfo[i].entry = entry;
        customerInfo[i].pizzas = pizzas;
        customerInfo[i].pizzaIDs = (int *)malloc(sizeof(int) * pizzas);
        for (int j = 0; j < pizzas; j++)
        {
            scanf("%d", &customerInfo[i].pizzaIDs[j]);
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "../colors.h"

int chefs, pizzaVars, limIngs, customers, ovens, time2ReachPickup;
int curTime;
pthread_mutex_t timeMutex;
pthread_cond_t timeCond;

struct pizza *pizzaInfo;
struct chef *chefInfo;

struct customer *customerInfo;
int *ingrAmt;

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
    int assignedPizza;
    pthread_mutex_t *mutex;
    pthread_t thread;
    sem_t *wakeUp;
};

struct orders
{
    int pizzas;
    int *pizzaIDs;
    struct customer *owner;
    pthread_t thread;
    sem_t *wakeUp;
};

struct customer
{
    int index;
    int entry;
    struct orders order;
    pthread_t thread;
    pthread_mutex_t *mutex;
    sem_t *wakeUp;
};

pthread_mutex_t pizzaLock;
pthread_mutex_t chefLock;
pthread_mutex_t customerLock;
pthread_mutex_t ingrLock;

sem_t driveQueue;
sem_t ovensQueue;

void *chefFunc(void *arg)
{
    struct chef *me = (struct chef *)arg;
}

void *ordersFunc(void *arg)
{
    struct orders *me = (struct orders *)arg;
    int totalAcceptedPizzas = 0;
    // go through chefs that are available rn
    for (int pizzaIdx = 0; pizzaIdx < me->pizzas; pizzaIdx++)
    {
        int assignedChef = 0;
        for (int i = 0; i < chefs; i++)
        {
            pthread_mutex_lock(chefInfo[i].mutex);
            if (chefInfo[i].entry <= curTime && chefInfo[i].exit > curTime + pizzaInfo[me->pizzaIDs[pizzaIdx]].t && chefInfo[i].assignedPizza == -1)
            {
                assignedChef = 1;
                totalAcceptedPizzas++;

                chefInfo[i].assignedPizza = me->pizzaIDs[pizzaIdx];

                sem_post(chefInfo[i].wakeUp);
            }
            pthread_mutex_unlock(chefInfo[i].mutex);

            if (assignedChef)
                break;
        }
    }

    for (int i = 0; i < totalAcceptedPizzas; i++)
    {
        sem_wait(me->wakeUp);
    }
}

void *customersFunc(void *arg)
{
    struct customer *me = (struct customer *)arg;
    sem_wait(me->wakeUp);
    printf("Customer %d arrives at time %d.\n", me->index + 1, curTime);

    // wait for entry
    sem_wait(&driveQueue);
    // create order thread
    pthread_create(&me->order.thread, NULL, ordersFunc, &me->order);
}

void *timerThread(void *arg)
{
    time_t start;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    start = ts.tv_sec;

    pthread_mutex_lock(&timeMutex);
    curTime = 0;
    pthread_mutex_unlock(&timeMutex);

    while (1)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        if (ts.tv_sec - start >= 1)
        {
            pthread_mutex_lock(&timeMutex);
            curTime++;
            pthread_cond_broadcast(&timeCond);
            // printf("In timerThread: %d\n", curTime);
            pthread_mutex_unlock(&timeMutex);
            start = ts.tv_sec;
        }
    }
}

// write customer compare
int customerCompare(const void *a, const void *b)
{
    int val1 = (((struct customer *)a)->entry - ((struct customer *)b)->entry);
    if (val1 == 0)
    {
        return (((struct customer *)a)->index - ((struct customer *)b)->index);
    }

    return val1;
}

int main()
{

    scanf("%d %d %d %d %d %d", &chefs, &pizzaVars, &limIngs, &customers, &ovens, &time2ReachPickup);
    pizzaInfo = (struct pizza *)malloc(sizeof(struct pizza) * pizzaVars);
    chefInfo = (struct chef *)malloc(sizeof(struct chef) * chefs);
    customerInfo = (struct customer *)malloc(sizeof(struct customer) * customers);
    ingrAmt = (int *)malloc(sizeof(int) * limIngs);
    sem_init(&driveQueue, 0, time2ReachPickup);
    sem_init(&ovensQueue, 0, ovens);

    curTime = -1;
    pthread_mutex_init(&timeMutex, NULL);
    pthread_cond_init(&timeCond, NULL);

    pthread_mutex_init(&pizzaLock, NULL);
    pthread_mutex_init(&chefLock, NULL);
    pthread_mutex_init(&customerLock, NULL);
    pthread_mutex_init(&ingrLock, NULL);

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
        chefInfo[i].assignedPizza = -1;
        chefInfo[i].mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        sem_init(chefInfo[i].wakeUp, 0, 0);
        pthread_mutex_init(chefInfo[i].mutex, NULL);
        pthread_create(&chefInfo[i].thread, NULL, chefFunc, &chefInfo[i]);
    }

    for (int i = 0; i < customers; i++)
    {
        // entry time, number of pizzas and then IDS of pizzas
        int entry, pizzas;
        scanf("%d %d", &entry, &pizzas);
        customerInfo[i].index = i;
        customerInfo[i].entry = entry;
        customerInfo[i].order.pizzas = pizzas;
        customerInfo[i].order.pizzaIDs = (int *)malloc(sizeof(int) * pizzas);
        customerInfo[i].mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        customerInfo[i].wakeUp = (sem_t *)malloc(sizeof(sem_t));

        pthread_mutex_init(customerInfo[i].mutex, NULL);
        sem_init(customerInfo[i].wakeUp, 0, 0);
        sem_init(&customerInfo[i].order.wakeUp, 0, 0);
        for (int j = 0; j < pizzas; j++)
        {
            scanf("%d", &customerInfo[i].order.pizzaIDs[j]);
        }

        customerInfo[i].order.owner = &customerInfo[i];
    }

    qsort(customerInfo, customers, sizeof(struct customer), customerCompare);

    // create all customer threads
    for (int i = 0; i < customers; i++)
    {
        pthread_create(&customerInfo[i].thread, NULL, customersFunc, (void *)&customerInfo[i]);
    }
    printf("Simulation Started\n");
    // timer thread
    pthread_t timer;
    pthread_create(&timer, NULL, timerThread, NULL);

    // REST OF THE CODE
    for (int i = 0; i < customers;)
    {

        pthread_mutex_lock(&timeMutex);
        pthread_mutex_lock(customerInfo[i].mutex);
        if (customerInfo[i].entry <= curTime)
        {
            sem_post(customerInfo[i].wakeUp);
            pthread_mutex_unlock(customerInfo[i++].mutex);
            pthread_mutex_unlock(&timeMutex);
            usleep(1);
            continue;
        }
        pthread_mutex_unlock(customerInfo[i].mutex);
        pthread_mutex_unlock(&timeMutex);
    }
    for (int i = 0; i < customers; i++)
    {
        pthread_join(customerInfo[i].thread, NULL);
    }
    pthread_cancel(timer);

    printf("Simulation Ended\n");
}

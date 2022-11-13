#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "../colors.h"

int chefs, pizzaVars, limIngs, customers, ovens, time2ReachPickup, s = 10;
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
    int index;
    int exit;
    int assignedPizza;
    struct orders *callBackOrder;
    pthread_mutex_t *mutex;
    pthread_t thread;
    sem_t *wakeUp;
};

struct orders
{
    int pizzas;
    int *pizzaIDs;
    int *results;
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
    int dirRejected;
    sem_t *wakeUp;
    sem_t *leaveQ;
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

    pthread_mutex_lock(&timeMutex);
    while (curTime < me->entry)
    {
        pthread_cond_wait(&timeCond, &timeMutex);
    }
    pthread_mutex_unlock(&timeMutex);

    printf("Chef %d arrives at time %d.\n", me->index, curTime);

    while (1)
    {
        // printf("Exit of Chef %d is %d and I should stay for %d\n", me->index, me->exit, me->exit - curTime);
        // get current time
        time_t start;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        start = ts.tv_sec;
        int res = sem_timedwait(me->wakeUp, &(struct timespec){.tv_sec = me->exit - curTime + start, .tv_nsec = 0});
        // terminate if time is up
        if (res == -1 && errno == ETIMEDOUT)
        {
            if (me->callBackOrder)
                sem_post(me->callBackOrder->wakeUp);

            printf("Chef %d exits at time here %d.\n", me->index, curTime);
            return NULL;
        }

        // check of ingrds
        pthread_mutex_lock(&(ingrLock));

        int enoughIngr = 1;
        for (int ingrIdx = 0; ingrIdx < limIngs; ingrIdx++)
        {
            if (ingrAmt[ingrIdx] < pizzaInfo[me->assignedPizza - 1].ingr[ingrIdx])
            {
                enoughIngr = 0;
                break;
            }
        }

        sleep(3);
        // use ingrs
        if (enoughIngr)
        {
            for (int ingrIdx = 0; ingrIdx < limIngs; ingrIdx++)
            {
                ingrAmt[ingrIdx] -= pizzaInfo[me->assignedPizza - 1].ingr[ingrIdx];
            }
        }
        else
        {
            pthread_mutex_unlock(&(ingrLock));
            sem_post(me->callBackOrder->wakeUp);
            me->assignedPizza = -1;
            me->callBackOrder = NULL;
            continue;
        }

        pthread_mutex_unlock(&(ingrLock));
        clock_gettime(CLOCK_REALTIME, &ts);
        start = ts.tv_sec;

        res = sem_timedwait(&(ovensQueue), &(struct timespec){.tv_sec = me->exit - curTime + start, .tv_nsec = 0});
        // terminate if time is up
        if (res == -1 && errno == ETIMEDOUT)
        {
            sem_post(me->callBackOrder->wakeUp);
            printf("Chef %d exits at time %d.\n", me->index, curTime);
            return NULL;
        }

        // cook pizza
        // check if time left
        if (me->exit - curTime < pizzaInfo[me->assignedPizza - 1].t)
        {
            sem_post(&(ovensQueue));
            sem_post(me->callBackOrder->wakeUp);
            me->assignedPizza = -1;
            me->callBackOrder = NULL;
            continue;
        }

        sleep(pizzaInfo[me->assignedPizza].t - 3);
        sem_post(&(ovensQueue));
        green();
        printf("Chef %d has picked up the pizza %d for the order %d from the oven at time %d.\n", me->index, me->assignedPizza, me->callBackOrder->owner->index, curTime);
        reset();
        // set result of callback
        me->callBackOrder->results[me->assignedPizza - 1] = 1;
        sem_post(me->callBackOrder->wakeUp);
        me->assignedPizza = -1;
        me->callBackOrder = NULL;
    }

    printf("Chef %d exits at time %d.\n", me->index, curTime);
    return NULL;
}

void *ordersFunc(void *arg)
{
    struct orders *me = (struct orders *)arg;
    int totalAcceptedPizzas = 0;
    printf("Order %d placed by customer %d has pizzas {", me->owner->index, me->owner->index);
    for (int i = 0; i < me->pizzas; i++)
    {
        printf("%d", me->pizzaIDs[i]);
        if (i != me->pizzas - 1)
        {
            printf(", ");
        }
        else
        {
            printf("}.\nOrder %d placed by customer %d awaits processing.\n", me->owner->index, me->owner->index);
        }
    }
    // go through chefs that are available rn
    for (int pizzaIdx = 0; pizzaIdx < me->pizzas; pizzaIdx++)
    {
        int assignedChef = 0;
        // check if enough ingredients
        pthread_mutex_lock(&ingrLock);
        int enoughIngr = 1;
        for (int ingrIdx = 0; ingrIdx < limIngs; ingrIdx++)
        {
            if (ingrAmt[ingrIdx] < pizzaInfo[me->pizzaIDs[pizzaIdx] - 1].ingr[ingrIdx])
            {
                enoughIngr = 0;
                break;
            }
        }
        pthread_mutex_unlock(&ingrLock);

        if (!enoughIngr)
        {
            continue;
        }

        for (int i = 0; i < chefs; i++)
        {
            pthread_mutex_lock(chefInfo[i].mutex);
            if (chefInfo[i].entry <= curTime && chefInfo[i].exit > curTime + pizzaInfo[me->pizzaIDs[pizzaIdx] - 1].t && chefInfo[i].assignedPizza == -1)
            {
                printf("Pizza %d in order %d assigned to chef %d.\n", me->pizzaIDs[pizzaIdx], me->owner->index, chefInfo[i].index);
                assignedChef = 1;
                totalAcceptedPizzas++;

                chefInfo[i].callBackOrder = me;
                chefInfo[i].assignedPizza = me->pizzaIDs[pizzaIdx];

                sem_post(chefInfo[i].wakeUp);
            }
            pthread_mutex_unlock(chefInfo[i].mutex);

            if (assignedChef)
                break;
        }
    }

    if (totalAcceptedPizzas == 0)
    {
        me->owner->dirRejected = 1;
        sem_post(me->owner->wakeUp); // leave

        return NULL;
    }

    sem_post(me->owner->wakeUp); // start going to queue
    for (int i = 0; i < totalAcceptedPizzas; i++)
    {
        sem_wait(me->wakeUp);
    }

    int anythingDone = 0;
    for (int i = 0; i < me->pizzas; i++)
    {
        if (me->results[i])
        {
            anythingDone = 1;
            break;
        }
    }

    if (anythingDone)
    {
        printf("Order %d placed by customer %d is ready.\n", me->owner->index, me->owner->index);
    }
    else
    {
        printf("Order %d placed by customer %d is rejected.\n", me->owner->index, me->owner->index);
        me->owner->dirRejected = 1;
        sem_post(me->owner->leaveQ);
        return NULL;
    }

    sem_post(me->owner->wakeUp);
}

void *customersFunc(void *arg)
{
    struct customer *me = (struct customer *)arg;
    sem_wait(me->wakeUp);
    printf("Customer %d arrives at time %d.\n", me->index + 1, curTime);

    // wait for entry
    sem_wait(&driveQueue);
    // create order thread
    int orderTime = curTime;
    pthread_create(&me->order.thread, NULL, ordersFunc, &me->order);

    sem_wait(me->wakeUp);

    if (me->dirRejected)
    {
        printf("Customer %d leaves at time %d.\n", me->index + 1, curTime);
        return NULL;
    }

    int time2sleep = time2ReachPickup - (curTime - orderTime);
    time_t start;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    start = ts.tv_sec;

    int shudILeave = sem_timedwait(me->leaveQ, &(struct timespec){.tv_sec = time2sleep + start, .tv_nsec = 0});

    if (shudILeave == 0)
    {
        printf("Customer %d leaves at time %d.\n", me->index + 1, curTime);
        return NULL;
    }

    printf("Customer %d is waiting at the pickup spot.\n", me->index + 1);
    // TODO: fix if rejected here?

    sem_wait(me->wakeUp);

    for (int i = 0; i < me->order.pizzas; i++)
    {
        if (me->order.results[i])
        {
            printf("Customer %d picks up their pizza %d.\n", me->index + 1, me->order.pizzaIDs[i]);
        }
    }
    printf("Customer %d exits the drive-thru zone.\n", me->index + 1);
}

void *timerThread(void *arg)
{
    time_t start;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    start = ts.tv_sec;

    pthread_mutex_lock(&timeMutex);
    curTime = 0;
    pthread_cond_broadcast(&timeCond);
    pthread_mutex_unlock(&timeMutex);

    while (1)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        if (ts.tv_sec - start >= 1)
        {
            pthread_mutex_lock(&timeMutex);
            curTime++;
            // printf("\n\n|%d|\n\n ", curTime);
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
    sem_init(&driveQueue, 0, customers);
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

        pizzaInfo[pizzaID - 1].ingr = (int *)malloc(sizeof(int) * limIngs);
        // set to 0
        for (int j = 0; j < limIngs; j++)
        {
            pizzaInfo[pizzaID - 1].ingr[j] = 0;
        }
        for (int j = 0; j < specIng; j++)
        {
            int ingrNo;
            scanf("%d", &ingrNo);
            pizzaInfo[pizzaID - 1].ingr[ingrNo - 1]++;
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
        chefInfo[i].index = i;
        chefInfo[i].entry = entry;
        chefInfo[i].exit = exit;
        chefInfo[i].assignedPizza = -1;
        chefInfo[i].callBackOrder = NULL;
        chefInfo[i].mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        chefInfo[i].wakeUp = (sem_t *)malloc(sizeof(sem_t));
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
        customerInfo[i].leaveQ = (sem_t *)malloc(sizeof(sem_t));
        customerInfo[i].dirRejected = 0;
        customerInfo[i].order.wakeUp = (sem_t *)malloc(sizeof(sem_t));

        pthread_mutex_init(customerInfo[i].mutex, NULL);
        sem_init(customerInfo[i].wakeUp, 0, 0);
        sem_init(customerInfo[i].leaveQ, 0, 0);
        sem_init(customerInfo[i].order.wakeUp, 0, 0);
        // setup results
        customerInfo[i].order.results = (int *)malloc(sizeof(int) * pizzas);
        for (int j = 0; j < pizzas; j++)
        {
            scanf("%d", &customerInfo[i].order.pizzaIDs[j]);
            customerInfo[i].order.results[j] = 0;
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

    // wait for chefs
    for (int i = 0; i < chefs; i++)
    {
        pthread_join(chefInfo[i].thread, NULL);
    }

    pthread_cancel(timer);

    printf("Simulation Ended\n");
}

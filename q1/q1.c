#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "../colors.h"

struct queueNode
{
    int data;
    struct queueNode *next;
};

struct queue
{
    struct queueNode *front;
    struct queueNode *rear;
    int size;
};

void initQueue(struct queue *q)
{
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}

void enqueue(struct queue *q, int data)
{
    struct queueNode *newNode = (struct queueNode *)malloc(sizeof(struct queueNode));
    newNode->data = data;
    newNode->next = NULL;
    if (q->front == NULL)
    {
        q->front = newNode;
        q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
}

int dequeue(struct queue *q)
{
    if (q->front == NULL)
    {
        printf("Queue is empty\n");
        return -1;
    }

    struct queueNode *temp = q->front;
    int data = temp->data;
    q->front = q->front->next;
    free(temp);
    q->size--;

    if (q->front == NULL)
    {
        q->rear = NULL;
    }

    return data;
}

struct queue *waiting;

int washingMachines;
pthread_mutex_t washingMachinesMutex;

void func1(void)
{
    printf("I am someone\n");
    sleep(1);
}

struct student
{
    int index;
    int T; // T is the time at which student arrives
    int W; // W is the time required by student to use the washing machine
    int P; // P is the time after which student will leave without using the washing machine
    pthread_mutex_t *mutex;
};

int cmpfunc(const void *a, const void *b)
{
    int val1 = (((struct student *)a)->T - ((struct student *)b)->T);
    if (val1 == 0)
    {
        return (((struct student *)a)->index - ((struct student *)b)->index);
    }

    return val1;
}

int main()
{
    int n, m; // n is the number of students, m is the number of washing machines
    scanf("%d %d", &n, &m);

    waiting = (struct queue *)malloc(sizeof(struct queue));

    if (waiting == NULL)
    {
        printf("Error in allocating memory for waiting queue.\n");
        exit(1);
    }

    initQueue(waiting);

    pthread_mutex_init(&washingMachinesMutex, NULL);

    pthread_mutex_lock(&washingMachinesMutex);
    washingMachines = m;
    pthread_mutex_unlock(&washingMachinesMutex);

    struct student *students = (struct student *)malloc(n * sizeof(struct student));

    // error check
    if (students == NULL)
    {
        printf("Error allocating memory\n");
        exit(1);
    }

    for (int i = 0; i < n; i++)
    {
        students[i].index = i;
        scanf("%d %d %d", &students[i].T, &students[i].W, &students[i].P);

        students[i].mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        if (students[i].mutex == NULL)
        {
            printf("Error allocating memory\n");
            exit(1);
        }

        pthread_mutex_init(students[i].mutex, NULL);
    }

    // sort the students array on T
    qsort(students, n, sizeof(struct student), cmpfunc);

    // find max time till which we need to simulate
    // int max = 0;
    // for (int i = 0; i < n; i++)
    // {
    //     if (students[i].T + students[i].P > max)
    //     {
    //         max = students[i].T + students[i].P;
    //     }
    //
    //     if (students[i].T + students[i].W > max)
    //     {
    //         max = students[i].T + students[i].W;
    //     }
    // }

    int curTime = 0;
    for (int i = 0; i < n; i++)
    {
        // sleep if time left
        if (students[i].T > curTime)
        {
            sleep(students[i].T - curTime);
            curTime = students[i].T;
        }

        enqueue(waiting, i);

        int toRun = waiting->front->data;
        // get lock
        pthread_mutex_lock(&washingMachinesMutex);
        if (washingMachines > 0)
        {
            // run
            washingMachines--;
            pthread_mutex_unlock(&washingMachinesMutex);

            // unlock the student
            pthread_mutex_unlock(students[toRun].mutex);

            // remove from waiting queue
            dequeue(waiting);
        }
        else
        {
            pthread_mutex_unlock(&washingMachinesMutex);
        }
    }

    // destroy
    pthread_mutex_destroy(&washingMachinesMutex);
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(students[i].mutex);
    }

    return 0;
}

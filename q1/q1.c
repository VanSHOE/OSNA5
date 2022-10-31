#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include "../colors.h"

int curTime;
pthread_mutex_t timeMutex;

struct queueNode
{
    int data;
    struct queueNode *next;
};

struct queue
{
    struct queueNode *front;
    struct queueNode *rear;
    pthread_mutex_t waitingMutex;
    int size;
};

void initQueue(struct queue *q)
{
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    pthread_mutex_init(&q->waitingMutex, NULL);
}

void enqueue(struct queue *q, int data)
{
    // get lock
    pthread_mutex_lock(&q->waitingMutex);
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
    // release lock
    pthread_mutex_unlock(&q->waitingMutex);
}

int dequeue(struct queue *q)
{
    // get lock
    pthread_mutex_lock(&q->waitingMutex);
    if (q->front == NULL)
    {
        printf("Queue is empty\n");
        pthread_mutex_unlock(&q->waitingMutex);
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

    // release lock
    pthread_mutex_unlock(&q->waitingMutex);

    return data;
}

struct queue *waiting;

int washingMachines;
pthread_mutex_t washingMachinesMutex;

struct student
{
    int index;
    int T; // T is the time at which student arrives
    int W; // W is the time required by student to use the washing machine
    int P; // P is the time after which student will leave without using the washing machine
    pthread_mutex_t *mutex;
    pthread_t thread;
};
struct student *students;

void *studentIn(void *arg)
{
    struct student *studentInfo = (struct student *)arg;

    free(studentInfo);
}

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

    pthread_mutex_init(&timeMutex, NULL);

    initQueue(waiting);

    pthread_mutex_init(&washingMachinesMutex, NULL);
    pthread_mutex_lock(&washingMachinesMutex);
    washingMachines = m;
    pthread_mutex_unlock(&washingMachinesMutex);

    students = (struct student *)malloc(n * sizeof(struct student));

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
    int max = 0;
    for (int i = 0; i < n; i++)
    {
        if (students[i].T + students[i].P > max)
        {
            max = students[i].T + students[i].P;
        }

        if (students[i].T + students[i].W > max)
        {
            max = students[i].T + students[i].W;
        }
    }

    curTime = 0;

    for (int i = 0; i < n; i++)
    {
        struct student *threadInfo = (struct student *)malloc(sizeof(struct student));
        threadInfo->index = students[i].index;
        threadInfo->T = students[i].T;
        threadInfo->W = students[i].W;
        threadInfo->P = students[i].P;
        threadInfo->mutex = students[i].mutex;
        pthread_create(&students[i].thread, NULL, studentIn, (void *)threadInfo);
    }

    for (int t = 0; t <= max; t++)
    {
        pthread_mutex_lock(&timeMutex);
        curTime = t;
        pthread_mutex_unlock(&timeMutex);
        sleep(1);
    }
    // join all
    for (int i = 0; i < n; i++)
    {
        pthread_join(students[i].thread, NULL);
    }

    // destroy
    pthread_mutex_destroy(&washingMachinesMutex);
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(students[i].mutex);
    }

    return 0;
}

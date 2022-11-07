#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "../colors.h"

int curTime;
pthread_mutex_t timeMutex;
pthread_cond_t timeCond;

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
        // printf("Queue is empty\n");
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
    // int status; // 0 for waiting to be created, 1 for waiting for washing machine, 2 for using washing machine, 3 for done
    pthread_mutex_t *mutex;
    pthread_t thread;
    sem_t *wakeUp;
    int invalid;
};
struct student *students;

void *studentIn(void *arg)
{
    struct student *studentInfo = (struct student *)arg;
    // wait on condition variable

    // wait for entry
    // printf("Student %d reporting for duty\n", studentInfo->index);

    sem_wait(studentInfo->wakeUp);

    printf("%d: Student %d arrives\n", curTime, studentInfo->index + 1);

    // wait for washing machine
    // get clockrealtime time
    time_t start;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    start = ts.tv_sec;
    // printf("Start time: %ld\n", start);
    struct timespec endTime;
    endTime.tv_sec = start + studentInfo->P + 1;
    endTime.tv_nsec = 0;

    int result = sem_timedwait(studentInfo->wakeUp, &endTime);

    if (result == -1 && errno == ETIMEDOUT)
    {
        red();
        printf("%d: Student %d leaves without washing\n", curTime, studentInfo->index + 1);
        reset();
        studentInfo->invalid = 1;
        return NULL;
    }

    // use washing machine
    printf("%d: Student %d starts washing for %d seconds.\n", curTime, studentInfo->index + 1, studentInfo->W);
    // sleep(studentInfo->W);
    int timePassed = 0;
    // use cond variable timeCond
    while (timePassed < studentInfo->W)
    {
        pthread_mutex_lock(&timeMutex);
        pthread_cond_wait(&timeCond, &timeMutex);
        timePassed++;
        pthread_mutex_unlock(&timeMutex);
    }

    // release washing machine
    printf("%d: Student %d leaves after washing\n", curTime - 1, studentInfo->index + 1);
    pthread_mutex_lock(&washingMachinesMutex);
    washingMachines++;
    pthread_mutex_unlock(&washingMachinesMutex);
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

void *queueThread(void *arg)
{
    // activate thread in the front
    while (1)
    {
        pthread_mutex_lock(&timeMutex);
        int curTimeCopy = curTime;
        pthread_mutex_unlock(&timeMutex);
        // printf("Trying lock\n");
        pthread_mutex_lock(&washingMachinesMutex);
        // printf("Got lock\n");
        if (!washingMachines)
        {
            pthread_mutex_unlock(&washingMachinesMutex);
            continue;
        }

        // printf("Yaar\n");
        int data = dequeue(waiting);
        if (data == -1 || students[data].invalid)
        {
            pthread_mutex_unlock(&washingMachinesMutex);
            continue;
        }
        // printf("Something\n");
        // lock
        // printf("Trying lock\n");
        // printf("Got lock\n");
        // printf("Signalling student %d\n", students[data].index);
        sem_post(students[data].wakeUp);
        washingMachines--;
        pthread_mutex_unlock(&washingMachinesMutex);
    }
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

    pthread_cond_init(&timeCond, NULL);

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
        students[i].wakeUp = (sem_t *)malloc(sizeof(sem_t));
        if (students[i].mutex == NULL)
        {
            printf("Error allocating memory\n");
            exit(1);
        }

        pthread_mutex_init(students[i].mutex, NULL);
        // init sem
        sem_init(students[i].wakeUp, 0, 0);
        students[i].invalid = 0;
    }

    // printf("Inputting done\n");
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

    curTime = -1;

    for (int i = 0; i < n; i++)
    {
        struct student *threadInfo = &students[i];
        // threadInfo->index = students[i].index;
        // threadInfo->T = students[i].T;
        // threadInfo->W = students[i].W;
        // threadInfo->P = students[i].P;
        // threadInfo->mutex = students[i].mutex;
        // threadInfo->wakeUp = students[i].wakeUp;
        pthread_create(&students[i].thread, NULL, studentIn, (void *)threadInfo);
    }

    pthread_t timer;
    pthread_create(&timer, NULL, timerThread, NULL);

    pthread_t queueHandler;
    pthread_create(&queueHandler, NULL, queueThread, NULL);

    for (int i = 0; i < n;)
    {

        pthread_mutex_lock(&timeMutex);
        pthread_mutex_lock(students[i].mutex);
        if (students[i].T <= curTime)
        {
            enqueue(waiting, i);
            // printf("In main curTime: %d for student: %d\n", curTime, students[i].index);
            // students[i].status = 1;
            sem_post(students[i].wakeUp);
            pthread_mutex_unlock(students[i++].mutex);
            pthread_mutex_unlock(&timeMutex);
            continue;
        }
        pthread_mutex_unlock(students[i].mutex);
        pthread_mutex_unlock(&timeMutex);
    }

    for (int i = 0; i < n; i++)
    {
        pthread_join(students[i].thread, NULL);
    }

    // kill leftover threads
    pthread_cancel(timer);
    pthread_cancel(queueHandler);

    // destroy
    pthread_mutex_destroy(&washingMachinesMutex);
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(students[i].mutex);
    }

    return 0;
}

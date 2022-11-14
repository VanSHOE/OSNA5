#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "../colors.h"

sem_t washingMachines;

int couldntWash;
pthread_mutex_t couldntWashtMutex;
pthread_mutex_t printLock;

int totalSecondsWasted;
pthread_mutex_t totalSecondsWastedMutex;

int curTime;
pthread_mutex_t timeMutex;
pthread_cond_t timeCond;

// struct queue *waiting;

// int washingMachines;
// pthread_mutex_t washingMachinesMutex;
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

    sem_wait(studentInfo->wakeUp);

    time_t start;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    start = ts.tv_sec;
    pthread_mutex_lock(&printLock);
    printf("%d: Student %d arrives to get their clothes washed\n", curTime, studentInfo->index + 1);
    pthread_mutex_unlock(&printLock);

    pthread_mutex_lock(&timeMutex);
    int cameAt = curTime;
    pthread_mutex_unlock(&timeMutex);

    // wait for washing machine
    // get clockrealtime time

    struct timespec endTime;
    endTime.tv_sec = start + studentInfo->P + 1;
    endTime.tv_nsec = 0;

    int result = sem_timedwait(&washingMachines, &endTime);

    if ((result == -1 && errno == ETIMEDOUT) || curTime > studentInfo->P + cameAt)
    {
        red();
        pthread_mutex_lock(&printLock);
        printf("%d: Student %d leaves without washing\n", curTime - 1, studentInfo->index + 1);
        pthread_mutex_unlock(&printLock);
        reset();
        pthread_mutex_lock(&couldntWashtMutex);
        couldntWash++;
        pthread_mutex_unlock(&couldntWashtMutex);
        pthread_mutex_lock(&totalSecondsWastedMutex);
        totalSecondsWasted += studentInfo->P;
        pthread_mutex_unlock(&totalSecondsWastedMutex);

        studentInfo->invalid = 1;
        return NULL;
    }

    // use washing machine
    pthread_mutex_lock(&totalSecondsWastedMutex);
    totalSecondsWasted += curTime - cameAt;
    pthread_mutex_unlock(&totalSecondsWastedMutex);

    green();
    pthread_mutex_lock(&printLock);
    printf("%d: Student %d starts washing.\n", curTime, studentInfo->index + 1);
    pthread_mutex_unlock(&printLock);
    reset();
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
    yellow();
    pthread_mutex_lock(&printLock);
    printf("%d: Student %d leaves after washing\n", curTime, studentInfo->index + 1);
    pthread_mutex_unlock(&printLock);
    reset();

    sem_post(&washingMachines);
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

            pthread_mutex_unlock(&timeMutex);
            start = ts.tv_sec;
        }
    }
}

int main()
{
    int n, m; // n is the number of students, m is the number of washing machines
    scanf("%d %d", &n, &m);

    sem_init(&washingMachines, 0, m);

    pthread_mutex_init(&couldntWashtMutex, NULL);
    pthread_mutex_init(&totalSecondsWastedMutex, NULL);
    pthread_mutex_init(&timeMutex, NULL);
    pthread_mutex_init(&printLock, NULL);

    // initQueue(waiting);

    pthread_cond_init(&timeCond, NULL);

    students = (struct student *)malloc(n * sizeof(struct student));

    // error check
    if (students == NULL)
    {
        pthread_mutex_lock(&printLock);
        printf("Error allocating memory\n");
        pthread_mutex_unlock(&printLock);
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
            pthread_mutex_lock(&printLock);
            printf("Error allocating memory\n");
            pthread_mutex_unlock(&printLock);
            exit(1);
        }

        pthread_mutex_init(students[i].mutex, NULL);
        // init sem
        sem_init(students[i].wakeUp, 0, 0);
        students[i].invalid = 0;
    }

    // sort the students array on T
    qsort(students, n, sizeof(struct student), cmpfunc);

    curTime = -1;

    for (int i = 0; i < n; i++)
    {
        struct student *threadInfo = &students[i];
        pthread_create(&students[i].thread, NULL, studentIn, (void *)threadInfo);
    }

    pthread_t timer;
    pthread_create(&timer, NULL, timerThread, NULL);

    for (int i = 0; i < n;)
    {

        pthread_mutex_lock(&timeMutex);
        pthread_mutex_lock(students[i].mutex);
        if (students[i].T <= curTime)
        {
            sem_post(students[i].wakeUp);
            pthread_mutex_unlock(students[i++].mutex);
            pthread_mutex_unlock(&timeMutex);
            usleep(1);
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

    // destroy
    sem_destroy(&washingMachines);
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(students[i].mutex);
    }

    pthread_mutex_lock(&printLock);
    printf("%d\n%d\n", couldntWash, totalSecondsWasted);
    pthread_mutex_unlock(&printLock);

    if (4 * couldntWash >= n)
    {
        pthread_mutex_lock(&printLock);
        printf("Yes\n");
        pthread_mutex_unlock(&printLock);
    }
    else
    {
        pthread_mutex_lock(&printLock);
        printf("No\n");
        pthread_mutex_unlock(&printLock);
    }

    return 0;
}

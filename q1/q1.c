#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "../colors.h"

void func1(void)
{
    printf("func1\n");
}

struct student
{
    int T; // T is the time at which student arrives
    int W; // W is the time required by student to use the washing machine
    int P; // P is the time after which student will leave without using the washing machine
};
pthread_mutex_t *mutex;
// cmpfunc for struct student on T
int cmpfunc(const void *a, const void *b)
{
    return (((struct student *)a)->T - ((struct student *)b)->T);
}

int main()
{
    int n, m; // n is the number of students, m is the number of washing machines
    scanf("%d %d", &n, &m);

    // create m mutex malloc
    mutex = (pthread_mutex_t *)malloc(m * sizeof(pthread_mutex_t));

    if (mutex == NULL)
    {
        printf("malloc failed\n");
        exit(1);
    }

    // init
    for (int i = 0; i < m; i++)
    {
        pthread_mutex_init(&mutex[i], NULL);
    }

    struct student *students = (struct student *)malloc(n * sizeof(struct student));

    // error check
    if (students == NULL)
    {
        printf("Error allocating memory\n");
        exit(1);
    }

    for (int i = 0; i < n; i++)
    {
        scanf("%d %d %d", &students[i].T, &students[i].W, &students[i].P);
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

    for (int t = 0; t < max; t++)
    {
    }

    // destroy
    for (int i = 0; i < m; i++)
    {
        pthread_mutex_destroy(&mutex[i]);
    }

    return 0;
}

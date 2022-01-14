#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <math.h>
#include <mqueue.h>
#include <pthread.h>
#include <time.h>

struct phil
{
    int t_index;
};

enum
{
    THINKING,
    HUNGRY,
    EATING
} state[5];

pthread_mutex_t mutex;
pthread_cond_t cond[5];

void test(int i)
{
    if ((state[(i + 4) % 5] != EATING) && (state[i] == HUNGRY) && (state[(i + 1) % 5] != EATING))
    {
        state[i] = EATING;
        pthread_mutex_lock(&mutex);
        printf("philosopher %d started eating now \n", i);
        int num = (rand() % (5 - 1 + 1)) + 1;
        int seconds = 1000000 * num;
        clock_t start_time = clock();
        while (clock() < start_time + seconds)
        {
            //do nothing
        }
        printf("philosopher %d finished eating now \n", i);
    }
}

void pickup(int i)
{
    state[i] = HUNGRY;
    test(i);
    if (state[i] != EATING)
        pthread_cond_wait(&cond[i], &mutex);
}

void putdown(int i)
{
    state[i] = THINKING;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond[(i + 4)%5]);
    pthread_cond_signal(&cond[(i + 1)%5]);
    test((i + 4) % 5);
    test((i + 1) % 5);

}

static void *do_task(void *arg_ptr)
{
    int temp = ((struct phil *)arg_ptr)->t_index;

    do
    {
        int num;
        num = (rand() % (10 - 1 + 1)) + 1;
        int second = 1000000 * num;
        clock_t start_time = clock();
        while (clock() < start_time + second)
        {
            //do nothing
        }
        pickup(temp);

        putdown(temp);

    } while (1);
    pthread_exit(NULL);
}

int main()
{
    pthread_mutex_init(&mutex, NULL);
    pthread_t tids[5];
    struct phil phils[5];
    int ret;

    for (int i = 0; i < 5; i++)
    {
        pthread_cond_init(&cond[i], NULL);
        state[i] = THINKING;
        phils[i].t_index = i;
        ret = pthread_create(&(tids[i]), NULL, do_task, (void *)&(phils[i]));
        if (ret != 0)
        {
            printf("thread create failed \n");
            exit(1);
        }
    }

    for (int i = 0; i < 5; i++)
    {
        ret = pthread_join(tids[i], NULL);
        if (ret != 0)
        {
            printf("thread join failed \n");
            exit(0);
        }
    }
}
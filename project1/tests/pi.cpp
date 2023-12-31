#include "../lib/uthread.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

/* Default time slice in uSec */
#define DEFAULT_TIME_SLICE 1000

/* Maximum number of points we can estimate with */
#define MAX_POINTS 1000000

void *worker(void *arg)
{   
    printf("inside worker\n");
    /* Retrieve our thread ID */
    int my_tid = uthread_self();
    printf("after self\n");
    /* Extract the points per thread argument */
    int points_per_thread = *(int *)arg;

    /* Extract a random state for each individual thread */
    unsigned int rand_state = rand();

    /** 
     * Initialize a thread-local count of the number of random points which
     * fall within the unit circle (x^2 + y^2 < 1)
    */
    unsigned long local_cnt = 0;

    float x, y;
    
    /* Loop for point_per_thread number of iterations */
    for (int i = 0; i < points_per_thread; i++)
    {
        /* Generate random X & Y index between 0 and 1 */
        x = rand_r(&rand_state) / ((double)RAND_MAX + 1) * 2.0 - 1.0;
        y = rand_r(&rand_state) / ((double)RAND_MAX + 1) * 2.0 - 1.0;

        /* If the random point falls within the quarter circle of the unit circle */
        if (x * x + y * y < 1)
            local_cnt++;
    }

    /* Allocate a unsigned long pointer and assign local count to it to return to the parent */
    unsigned long *return_buffer = new unsigned long;
    *return_buffer = local_cnt;
    // cout<<"result is here:"<<*return_buffer<<endl;
    return return_buffer;
}

int main(int argc, char *argv[])
{

    /* Initialize the default time slice (only overridden if passed in) */
    int quantum_usecs = DEFAULT_TIME_SLICE;

    /* Verify the number of arguments passed in */
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./pi <total points> <threads> [quantum_usecs]\n");
        fprintf(stderr, "Example: ./pi 1000000 8\n");
        exit(-1);
    }
    else if (argc == 4)
    {
        quantum_usecs = atoi(argv[3]);
    }

    /* Extract arguments and determine workload for each thread */
    unsigned long total_points = atol(argv[1]);
    int thread_count = atoi(argv[2]);
    int *threads = new int[thread_count];
    int points_per_thread = total_points / thread_count;

    /* To avoid over-running the thread stack validate the max number of threads and monte carlo points */
    if(thread_count >= MAX_THREAD_NUM)
    {
        fprintf(stderr, "Too many threads selected. Please choose less than [%d]\n", MAX_THREAD_NUM);
        exit(-1);
    }

    if(total_points > MAX_POINTS)
    {
        fprintf(stderr, "Too many points. Please choose less than [%d]\n", MAX_POINTS);
        exit(-1);
    }


    /* Report header information */
    printf("\nStarting monte carlo Pi approximation:\n");
    printf("    Total Points:      [%ld]\n", total_points);
    printf("    # Threads:         [%d]\n", thread_count);
    printf("    Points per thread: [%d]\n", points_per_thread);
    if(quantum_usecs == DEFAULT_TIME_SLICE)
        printf("    Time Slice:        [%d] uSec (DEFAULT)\n\n", quantum_usecs);
    else
        printf("    Time Slice:        [%d] uSec\n\n", quantum_usecs);

    /* Initialize the user thread library */
    int ret = uthread_init(quantum_usecs);
    if (ret != 0)
    {
        cerr << "uthread_init FAIL!\n"
             << endl;
        exit(1);
    }

    srand(time(NULL));

    /* Create a thread pool of threads passing in the points per thread */
    for (int i = 0; i < thread_count; i++)
    {
        int tid = uthread_create(worker, &points_per_thread);
        threads[i] = tid;
        cout<<"all the id"<<tid<<endl;
        cout<<"all the state"<<endl;
    }
    printf("after create all the thread\n");

    // test_suspend();
    // test_resume();
    

    printf("Testing suspend function...\n");
    for (int i = 0; i < thread_count; i++)
    {
        int res_suspend = uthread_suspend(threads[i]);
        cout<<"Result "<<i<<": "<<res_suspend<<endl;
    }
    printf("Testing completed!\n");

    printf("Testing resume function...\n");
    for (int i = 0; i < thread_count; i++)
    {
        int res_resume = uthread_resume(threads[i]);
        cout<<"Result "<<i<<": "<<res_resume<<endl;
    }
    printf("Testing completed!\n");

    /* Join each thread and compute the result */
    unsigned long g_cnt = 0;
    for (int i = 0; i < thread_count; i++)
    {
        /* Collect the result from the user via their allocated heap pointer */
        unsigned long *local_cnt;
        cout<<"Local_cnt:"<<local_cnt<<endl;
        cout<<"each id"<<threads[i]<<endl;
        uthread_join(threads[i], (void **)&local_cnt);
        // if(*local_cnt==nullptr){
        //     printf("result is null\n");
        // }
        cout<<"Local_cnt:"<<local_cnt<<endl;
        /* Deallocate pointer to get the actual count*/
        g_cnt += *local_cnt;

        /* Deallocate thread result on the heap */
        delete local_cnt;
    }
    cout<<"final answer:"<<g_cnt<<endl;


    delete[] threads;

    /**
     * The monte carlo Pi estimation states that the ratio of points that lie in 1/4 of the unit circle
     * is Pi/4 points thus if we take our ration of points and multiple by 4 we get our estimation of Pi
     * which is then divided by the total number of points computed
     */
    printf("\nFinal Estimation of Pi: [%f]\n", (4. * (double)g_cnt) / ((double)points_per_thread * thread_count));

    return 0;
}

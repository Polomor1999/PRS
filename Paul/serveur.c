#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t mutex;

void *write_function(void *arg)
{
    int i;
    for (i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);
        printf("Thread %d : Writing...\n", (int)arg);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void)
{
    pthread_t thread1, thread2;
    int ret;

    // Initialize the mutex
    pthread_mutex_init(&mutex, NULL);

    // Create the threads
    ret = pthread_create(&thread1, NULL, write_function, (void *)1);
    if (ret != 0) {
        fprintf(stderr, "Error creating thread 1\n");
        exit(EXIT_FAILURE);
    }
    ret = pthread_create(&thread2, NULL, write_function, (void *)2);
    if (ret != 0) {
        fprintf(stderr, "Error creating thread 2\n");
        exit(EXIT_FAILURE);
    }

    // Wait for the threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // Destroy the mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}
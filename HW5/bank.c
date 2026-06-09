#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ITERATIONS 100000
#define NUM_DEPOSIT_THREADS 2
#define NUM_WITHDRAW_THREADS 2

int balance = 0;
pthread_mutex_t lock;

void *deposit(void *arg) { (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock);
        balance++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *withdraw(void *arg) { (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock);
        balance--;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t dep_threads[NUM_DEPOSIT_THREADS];
    pthread_t wdraw_threads[NUM_WITHDRAW_THREADS];

    pthread_mutex_init(&lock, NULL);

    printf("Initial balance: %d\n", balance);
    printf("Starting %d deposit threads (%d ops each) + %d withdraw threads (%d ops each)...\n",
           NUM_DEPOSIT_THREADS, ITERATIONS, NUM_WITHDRAW_THREADS, ITERATIONS);

    for (int i = 0; i < NUM_DEPOSIT_THREADS; i++)
        pthread_create(&dep_threads[i], NULL, deposit, NULL);
    for (int i = 0; i < NUM_WITHDRAW_THREADS; i++)
        pthread_create(&wdraw_threads[i], NULL, withdraw, NULL);

    for (int i = 0; i < NUM_DEPOSIT_THREADS; i++)
        pthread_join(dep_threads[i], NULL);
    for (int i = 0; i < NUM_WITHDRAW_THREADS; i++)
        pthread_join(wdraw_threads[i], NULL);

    pthread_mutex_destroy(&lock);

    printf("Final balance: %d\n", balance);
    if (balance == 0)
        printf("PASS: Balance is correct!\n");
    else
        printf("FAIL: Balance should be 0, got %d\n", balance);

    return 0;
}

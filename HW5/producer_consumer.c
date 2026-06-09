#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 10

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;
int total_produced = 0;
int total_consumed = 0;
int items_to_produce = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
int done = 0;

pthread_mutex_t mutex;
pthread_cond_t cond_not_full;
pthread_cond_t cond_not_empty;

void *producer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        usleep(rand() % 100000);

        pthread_mutex_lock(&mutex);

        if (total_produced >= items_to_produce) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        while (count == BUFFER_SIZE)
            pthread_cond_wait(&cond_not_full, &mutex);

        int item = id * 1000 + (total_produced++);
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("[Producer %d] item=%4d | buffer=%d/%d\n", id, item, count, BUFFER_SIZE);

        pthread_cond_broadcast(&cond_not_empty);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        usleep(rand() % 150000);

        pthread_mutex_lock(&mutex);

        if (total_consumed >= items_to_produce) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        while (count == 0) {
            if (total_consumed >= items_to_produce) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&cond_not_empty, &mutex);
        }

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        total_consumed++;
        printf("[Consumer %d] item=%4d | buffer=%d/%d\n", id, item, count, BUFFER_SIZE);

        pthread_cond_signal(&cond_not_full);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t prod_threads[NUM_PRODUCERS];
    pthread_t cons_threads[NUM_CONSUMERS];
    int prod_ids[NUM_PRODUCERS];
    int cons_ids[NUM_CONSUMERS];

    srand(time(NULL));
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_not_full, NULL);
    pthread_cond_init(&cond_not_empty, NULL);

    printf("=== Producer-Consumer Problem ===\n");
    printf("Buffer=%d | Producers=%d (%d items each) | Consumers=%d\n\n",
           BUFFER_SIZE, NUM_PRODUCERS, ITEMS_PER_PRODUCER, NUM_CONSUMERS);

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        prod_ids[i] = i + 1;
        pthread_create(&prod_threads[i], NULL, producer, &prod_ids[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cons_ids[i] = i + 1;
        pthread_create(&cons_threads[i], NULL, consumer, &cons_ids[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(prod_threads[i], NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_join(cons_threads[i], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    printf("\nSimulation complete. Produced: %d | Consumed: %d | Buffer left: %d\n",
           total_produced, total_consumed, count);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define MEALS 3  // each philosopher eats MEALS times

pthread_mutex_t chopsticks[NUM_PHILOSOPHERS];

void think(int id) {
    printf("Philosopher %d is thinking...\n", id);
    usleep(rand() % 200000);
}

void eat(int id) {
    printf("Philosopher %d is *** EATING ***\n", id);
    usleep(rand() % 150000);
}

void *philosopher(void *arg) {
    int id = *(int *)arg;
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    for (int i = 0; i < MEALS; i++) {
        think(id);

        // Even philosophers pick left first, odd pick right first.
        // This asymmetry breaks circular wait and prevents deadlock.
        pthread_mutex_t *first = (id % 2 == 0) ? &chopsticks[left] : &chopsticks[right];
        pthread_mutex_t *second = (id % 2 == 0) ? &chopsticks[right] : &chopsticks[left];

        pthread_mutex_lock(first);
        printf("Philosopher %d picked up chopstick %d\n",
               id, (first == &chopsticks[left]) ? left : right);

        pthread_mutex_lock(second);
        printf("Philosopher %d picked up chopstick %d\n",
               id, (second == &chopsticks[right]) ? right : left);

        eat(id);

        pthread_mutex_unlock(second);
        pthread_mutex_unlock(first);
        printf("Philosopher %d put down both chopsticks\n", id);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_PHILOSOPHERS];
    int ids[NUM_PHILOSOPHERS];

    srand(time(NULL));

    printf("=== Dining Philosophers Problem ===\n");
    printf("%d philosophers, %d meals each\n\n", NUM_PHILOSOPHERS, MEALS);

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&chopsticks[i], NULL);
        ids[i] = i;
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_create(&threads[i], NULL, philosopher, &ids[i]);

    for (int i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_join(threads[i], NULL);

    for (int i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_mutex_destroy(&chopsticks[i]);

    printf("\nAll philosophers finished their meals. No deadlock occurred.\n");
    return 0;
}

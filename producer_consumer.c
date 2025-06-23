
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define MAX_LINE_LENGTH 100

char *buffer[BUFFER_SIZE];
int in = 0, out = 0, count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

FILE *dataFile;

void produce(char *item) {
    pthread_mutex_lock(&mutex);
    while (count == BUFFER_SIZE) {
        pthread_cond_wait(&not_full, &mutex);
    }
    buffer[in] = strdup(item);
    in = (in + 1) % BUFFER_SIZE;
    count++;
    printf("Produced: %s\n", item);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

char* consume() {
    pthread_mutex_lock(&mutex);
    while (count == 0) {
        pthread_cond_wait(&not_empty, &mutex);
    }
    char *item = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
    count--;
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    return item;
}

void* producer(void *arg) {
    char line[MAX_LINE_LENGTH];
    while (1) {
        pthread_mutex_lock(&mutex);
        if (fgets(line, sizeof(line), dataFile) == NULL) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        line[strcspn(line, "\n")] = '\0';
        produce(line);
        usleep(100000 + rand() % 500000);
    }
    pthread_exit(NULL);
}

void* consumer(void *arg) {
    int id = *(int*)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        if (feof(dataFile) && count == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        char *item = consume();
        printf("Consumer %d consumed: %s\n", id, item);
        free(item);
        usleep(200000 + rand() % 500000);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t prod_threads[2], cons_threads[2];
    int cons_ids[2] = {1, 2};

    dataFile = fopen("input.txt", "r");
    if (!dataFile) {
        perror("Failed to open input.txt");
        return 1;
    }

    srand(time(NULL));

    for (int i = 0; i < 2; i++) {
        pthread_create(&prod_threads[i], NULL, producer, NULL);
    }

    for (int i = 0; i < 2; i++) {
        pthread_create(&cons_threads[i], NULL, consumer, &cons_ids[i]);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(prod_threads[i], NULL);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(cons_threads[i], NULL);
    }

    fclose(dataFile);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}

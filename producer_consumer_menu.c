
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define MAX_LINE_LENGTH 100

char *buffer[BUFFER_SIZE];
int in = 0, out = 0, count = 0;
int produced_count = 0, consumed_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

FILE *dataFile;
pthread_t prod_threads[2], cons_threads[2];
int cons_ids[2] = {1, 2};

int producers_started = 0, consumers_started = 0;

void produce(char *item) {
    pthread_mutex_lock(&mutex);
    while (count == BUFFER_SIZE) {
        pthread_cond_wait(&not_full, &mutex);
    }
    buffer[in] = strdup(item);
    in = (in + 1) % BUFFER_SIZE;
    count++;
    produced_count++;
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
    consumed_count++;
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

void showBufferStatus() {
    pthread_mutex_lock(&mutex);
    printf("\n==== Buffer Status ====\n");
    printf("Buffer slots used: %d/%d\n", count, BUFFER_SIZE);
    printf("Current items:\n");
    for (int i = 0; i < count; i++) {
        int index = (out + i) % BUFFER_SIZE;
        printf(" [%d]: %s\n", i + 1, buffer[index]);
    }
    if (count == 0) printf(" Buffer is empty.\n");
    printf("=======================\n\n");
    pthread_mutex_unlock(&mutex);
}

void showStatistics() {
    pthread_mutex_lock(&mutex);
    printf("\n==== Statistics ====\n");
    printf("Total produced: %d\n", produced_count);
    printf("Total consumed: %d\n", consumed_count);
    printf("====================\n\n");
    pthread_mutex_unlock(&mutex);
}

int main() {
    int choice;
    dataFile = fopen("input.txt", "r");
    if (!dataFile) {
        perror("Failed to open input.txt");
        return 1;
    }

    srand(time(NULL));

    while (1) {
        printf("\n==== Producer-Consumer Menu ====\n");
        printf("1. Start Producers\n");
        printf("2. Start Consumers\n");
        printf("3. Show Buffer Status\n");
        printf("4. Show Statistics\n");
        printf("5. Exit\n");
        printf("Choose option: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                if (!producers_started) {
                    for (int i = 0; i < 2; i++)
                        pthread_create(&prod_threads[i], NULL, producer, NULL);
                    producers_started = 1;
                    printf("Producers started.\n");
                } else {
                    printf("Producers already running.\n");
                }
                break;
            case 2:
                if (!consumers_started) {
                    for (int i = 0; i < 2; i++)
                        pthread_create(&cons_threads[i], NULL, consumer, &cons_ids[i]);
                    consumers_started = 1;
                    printf("Consumers started.\n");
                } else {
                    printf("Consumers already running.\n");
                }
                break;
            case 3:
                showBufferStatus();
                break;
            case 4:
                showStatistics();
                break;
            case 5:
                printf("Exiting program...\n");
                if (producers_started) {
                    for (int i = 0; i < 2; i++)
                        pthread_join(prod_threads[i], NULL);
                }
                if (consumers_started) {
                    for (int i = 0; i < 2; i++)
                        pthread_join(cons_threads[i], NULL);
                }
                fclose(dataFile);
                pthread_mutex_destroy(&mutex);
                pthread_cond_destroy(&not_full);
                pthread_cond_destroy(&not_empty);
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include <sys/file.h>

#define MAX_NAME 100
#define MAX_ADDRESS 200
#define MAX_ORDERS 100

typedef enum {PENDING, DELIVERED} Status;
typedef enum {EXPRESS, REGULER} Type;

typedef struct {
    char name[MAX_NAME];
    char address[MAX_ADDRESS];
    Type type;
    Status status;
    char agent[MAX_NAME];
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
    pthread_mutex_t lock;
} SharedData;

SharedData *shared_data;

void log_delivery(const char *agent, const char *type, const char *name, const char *address) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) return;
    flock(fileno(log), LOCK_EX);
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] %s package delivered to %s in %s\n",
        t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec,
        agent, type, name, address);
    fflush(log);
    flock(fileno(log), LOCK_UN);
    fclose(log);
}

void* agent_thread(void *arg) {
    char *agent = (char*) arg;

    while (1) {
        pthread_mutex_lock(&shared_data->lock);
        for (int i = 0; i < shared_data->count; i++) {
            if (shared_data->orders[i].type == EXPRESS && shared_data->orders[i].status == PENDING) {
                shared_data->orders[i].status = DELIVERED;
                strcpy(shared_data->orders[i].agent, agent);
                log_delivery(agent, "Express", shared_data->orders[i].name, shared_data->orders[i].address);
                break;
            }
        }
        pthread_mutex_unlock(&shared_data->lock);
        sleep(1);
    }
    return NULL;
}

Type parse_type(char *str) {
    if (strcasecmp(str, "Express") == 0) return EXPRESS;
    return REGULER;
}

void read_csv_and_store(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (shared_data->count >= MAX_ORDERS) break;
        char *name = strtok(line, ",");
        char *address = strtok(NULL, ",");
        char *type_str = strtok(NULL, "\n");

        if (!name || !address || !type_str) continue;

        Order *o = &shared_data->orders[shared_data->count++];
        strncpy(o->name, name, MAX_NAME);
        strncpy(o->address, address, MAX_ADDRESS);
        o->type = parse_type(type_str);
        o->status = PENDING;
        strcpy(o->agent, "-");
    }
    fclose(f);
}

int main() {
    key_t key = ftok("delivery_agent.c", 65);
    int shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    shared_data = (SharedData*) shmat(shmid, NULL, 0);

    shared_data->count = 0;
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_data->lock, &mattr);

    read_csv_and_store("delivery_order.csv");

    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, agent_thread, "AGENT A");
    pthread_create(&t2, NULL, agent_thread, "AGENT B");
    pthread_create(&t3, NULL, agent_thread, "AGENT C");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    shmdt(shared_data);
    return 0;
}


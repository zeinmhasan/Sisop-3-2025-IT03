#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pwd.h>
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

char* get_username() {
    struct passwd *pw = getpwuid(getuid());
    return pw->pw_name;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./dispatcher -deliver [Nama] | -status [Nama] | -list\n");
        return 1;
    }

    key_t key = ftok("delivery_agent.c", 65);
    int shmid = shmget(key, sizeof(SharedData), 0666);
    shared_data = (SharedData*) shmat(shmid, NULL, 0);

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        char *nama = argv[2];
        for (int i = 0; i < shared_data->count; i++) {
            if (strcmp(shared_data->orders[i].name, nama) == 0 && shared_data->orders[i].type == REGULER) {
                if (shared_data->orders[i].status == DELIVERED) {
                    printf("Order sudah dikirim.\n");
                    return 0;
                }
                shared_data->orders[i].status = DELIVERED;
                strcpy(shared_data->orders[i].agent, get_username());
                log_delivery(shared_data->orders[i].agent, "Reguler", nama, shared_data->orders[i].address);
                printf("Order untuk %s telah dikirim.\n", nama);
                return 0;
            }
        }
        printf("Order tidak ditemukan.\n");
    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        char *nama = argv[2];
        for (int i = 0; i < shared_data->count; i++) {
            if (strcmp(shared_data->orders[i].name, nama) == 0) {
                if (shared_data->orders[i].status == DELIVERED)
                    printf("Status for %s: Delivered by %s\n", nama, shared_data->orders[i].agent);
                else
                    printf("Status for %s: Pending\n", nama);
                return 0;
            }
        }
        printf("Order tidak ditemukan.\n");
    } else if (strcmp(argv[1], "-list") == 0) {
        for (int i = 0; i < shared_data->count; i++) {
            printf("%s - %s\n", shared_data->orders[i].name,
                shared_data->orders[i].status == DELIVERED ? "Delivered" : "Pending");
        }
    } else {
        printf("Perintah tidak dikenali.\n");
    }

    shmdt(shared_data);
    return 0;
}


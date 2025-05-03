#include "shm_common.h"
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>

void set_terminal_mode(int enable) {
    static struct termios oldt, newt;
    if (!enable) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    } else {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
}

struct Hunter *login_hunter(struct SystemData *sys) {
    char name[50];
    printf("Username: ");
    scanf("%s", name);

    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, name) == 0 && !sys->hunters[i].banned) {
            printf("[LOGIN] Welcome, %s!\n", name);
            return &sys->hunters[i];
        }
    }

    printf("Hunter not found or banned.\n");
    return NULL;
}

void register_hunter(struct SystemData *sys) {
    if (sys->num_hunters >= MAX_HUNTERS) {
        printf("Max hunter reached!\n");
        return;
    }

    char name[50];
    printf("Username: ");
    scanf("%s", name);

    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, name) == 0) {
            printf("Username taken!\n");
            return;
        }
    }

    struct Hunter h;
    strcpy(h.username, name);
    h.level = 1;
    h.exp = 0;
    h.atk = 10;
    h.hp = 100;
    h.def = 5;
    h.banned = 0;
    h.shm_key = rand();

    sys->hunters[sys->num_hunters++] = h;
    printf("Registration success!\n");
}

void list_available_dungeons(struct SystemData *sys, struct Hunter *h) {
    printf("=== AVAILABLE DUNGEONS ===\n");
    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon d = sys->dungeons[i];
        if (h->level >= d.min_level) {
            printf("%d. %s (Level %d+)\n", i + 1, d.name, d.min_level);
        }
    }
}

void raid_dungeon(struct SystemData *sys, struct Hunter *h) {
    list_available_dungeons(sys, h);
    int c;
    printf("Choose Dungeon: ");
    scanf("%d", &c); c--;

    if (c < 0 || c >= sys->num_dungeons || h->level < sys->dungeons[c].min_level) {
        printf("Invalid dungeon!\n");
        return;
    }

    struct Dungeon d = sys->dungeons[c];

    h->atk += d.atk;
    h->hp += d.hp;
    h->def += d.def;
    h->exp += d.exp;

    printf("Raid success! Gained:\nATK: %d\nHP: %d\nDEF: %d\nEXP: %d\n", d.atk, d.hp, d.def, d.exp);

    if (h->exp >= 500) {
        h->level++;
        h->exp = 0;
        printf("Level up! Now Level %d\n", h->level);
    }

    for (int i = c; i < sys->num_dungeons - 1; i++) {
        sys->dungeons[i] = sys->dungeons[i + 1];
    }
    sys->num_dungeons--;
}

void battle_hunter(struct SystemData *sys, struct Hunter *h) {
    printf("=== PVP LIST ===\n");
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(h->username, sys->hunters[i].username) != 0 && !sys->hunters[i].banned) {
            int pow = sys->hunters[i].atk + sys->hunters[i].hp + sys->hunters[i].def;
            printf("%s - Total Power: %d\n", sys->hunters[i].username, pow);
        }
    }

    char target[50];
    printf("Target: ");
    scanf("%s", target);

    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, target) == 0) {
            int my_pow = h->atk + h->hp + h->def;
            int their_pow = sys->hunters[i].atk + sys->hunters[i].hp + sys->hunters[i].def;

            if (my_pow >= their_pow) {
                printf("You won the battle!\n");
                h->atk += sys->hunters[i].atk;
                h->hp += sys->hunters[i].hp;
                h->def += sys->hunters[i].def;

                for (int j = i; j < sys->num_hunters - 1; j++) {
                    sys->hunters[j] = sys->hunters[j + 1];
                }
                sys->num_hunters--;
            } else {
                printf("You lost the battle!\n");

                sys->hunters[i].atk += h->atk;
                sys->hunters[i].hp += h->hp;
                sys->hunters[i].def += h->def;

                for (int j = 0; j < sys->num_hunters; j++) {
                    if (strcmp(sys->hunters[j].username, h->username) == 0) {
                        for (int k = j; k < sys->num_hunters - 1; k++) {
                            sys->hunters[k] = sys->hunters[k + 1];
                        }
                        sys->num_hunters--;
                        break;
                    }
                }
            }
            return;
        }
    }

    printf("Target not found.\n");
}

void show_notifications(struct Hunter *hunter, struct SystemData *sys) {
    printf("\n--- NOTIFICATION MODE ---\n");
    printf("Press 'q' then ENTER to exit notifications.\n");

    set_terminal_mode(1);
    int index = 0;

    while (1) {
        system("clear");

        printf("=== HUNTER SYSTEM ===\n");

        if (sys->num_dungeons == 0) {
            printf("No dungeons available.\n");
        } else {
            int count = 0;
            for (int i = 0; i < sys->num_dungeons; i++) {
                if (sys->dungeons[i].min_level <= hunter->level) count++;
            }

            if (count == 0) {
                printf("No dungeon available for your level yet.\n");
            } else {
                index = index % sys->num_dungeons;
                struct Dungeon *d = &sys->dungeons[index];
                if (d->min_level <= hunter->level) {
                    printf("%s for minimum level %d opened!\n", d->name, d->min_level);
                } else {
                    printf("Waiting for new dungeon unlocks...\n");
                }
                index++;
            }
        }

        printf("=== %s's MENU ===\n", hunter->username);
        printf("1. Dungeon List\n2. Dungeon Raid\n3. Hunter's Battle\n4. Notification\n5. Exit\n");
        printf("Press 'q' to return to main menu...\n");

        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int ret = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);

        if (ret > 0) {
            char c = getchar();
            if (c == 'q' || c == 'Q') {
                break;
            }
        }
    }

    set_terminal_mode(0);
}

void cleanup_shared_memory(int shmid, void *shm_ptr) {
    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    printf("[INFO] Shared memory cleaned up.\n");
}

void signal_handler(int sig) {
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), 0666);
    if (shmid != -1) {
        void *shm_ptr = shmat(shmid, NULL, 0);
        if (shm_ptr != (void *) -1) {
            cleanup_shared_memory(shmid, shm_ptr);
        }
    }
    exit(0);
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    struct SystemData *sys = shmat(shmid, NULL, 0);
    if (sys == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }

    int choice;
    struct Hunter *login = NULL;

    while (1) {
        if (!login) {
            printf("=== HUNTER SYSTEM ===\n1. Register\n2. Login\n3. Exit\nChoice: ");
            scanf("%d", &choice);
            if (choice == 1) register_hunter(sys);
            else if (choice == 2) login = login_hunter(sys);
            else break;
        } else {
            printf("\n=== %s's MENU ===\n", login->username);
            printf("1. Dungeon List\n2. Dungeon Raid\n3. Hunter's Battle\n4. Notification\n5. Exit\nChoice: ");
            scanf("%d", &choice);

            if (choice == 1) list_available_dungeons(sys, login);
            else if (choice == 2) raid_dungeon(sys, login);
            else if (choice == 3) battle_hunter(sys, login);
            else if (choice == 4) show_notifications(login, sys);
            else login = NULL;
        }
    }

    cleanup_shared_memory(shmid, sys);
    return 0;
}


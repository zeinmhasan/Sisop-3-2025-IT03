#include "shm_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

char *dungeon_names[] = {
    "Double Dungeon", "Demon Castle", "Pyramid Dungeon", "Red Gate Dungeon",
    "Hunters Guild Dungeon", "Busan A-Rank Dungeon", "Insects Dungeon",
    "Goblins Dungeon", "D-Rank Dungeon", "Gwanak Mountain Dungeon",
    "Hapjeong Subway Station Dungeon"
};

void print_hunters(struct SystemData *sys) {
    printf("\n=== HUNTER INFO ===\n");
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *h = &sys->hunters[i];
        printf("Name: %s\tLevel: %d\tEXP: %d\tATK: %d\tHP: %d\tDEF: %d\tStatus: %s\n",
            h->username, h->level, h->exp, h->atk, h->hp, h->def,
            h->banned ? "BANNED" : "Active");
    }
}

void print_dungeons(struct SystemData *sys) {
    printf("\n=== DUNGEON INFO ===\n");
    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon *d = &sys->dungeons[i];
        printf("[Dungeon %d]\n", i + 1);
        printf("Name: %s\n", d->name);
        printf("Minimum Level: %d\n", d->min_level);
        printf("EXP Reward: %d\n", d->exp);
        printf("ATK: %d\n", d->atk);
        printf("HP: %d\n", d->hp);
        printf("DEF: %d\n", d->def);
        printf("Key: %d\n\n", d->shm_key);
    }
}

void generate_dungeon(struct SystemData *sys) {
    if (sys->num_dungeons >= MAX_DUNGEONS) {
        printf("Dungeon limit reached.\n");
        return;
    }

    struct Dungeon *d = &sys->dungeons[sys->num_dungeons];

    strcpy(d->name, dungeon_names[rand() % (sizeof(dungeon_names)/sizeof(char*))]);
    d->min_level = (rand() % 5) + 1;
    d->exp = (rand() % 151) + 150;
    d->atk = (rand() % 51) + 100;
    d->hp = (rand() % 51) + 50;
    d->def = (rand() % 26) + 25;
    d->shm_key = rand();

    sys->num_dungeons++;
    printf("\nDungeon generated!\nName: %s\nMinimum Level: %d\n", d->name, d->min_level);
}

void ban_hunter(struct SystemData *sys) {
    char name[50];
    printf("Enter hunter name to ban: ");
    scanf("%s", name);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, name) == 0) {
            sys->hunters[i].banned = 1;
            printf("Hunter %s has been banned.\n", name);
            return;
        }
    }
    printf("Hunter not found.\n");
}

void unban_hunter(struct SystemData *sys) {
    char name[50];
    printf("Enter hunter name to unban: ");
    scanf("%s", name);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, name) == 0) {
            sys->hunters[i].banned = 0;
            printf("Hunter %s has been unbanned.\n", name);
            return;
        }
    }
    printf("Hunter not found.\n");
}

void reset_hunter(struct SystemData *sys) {
    char name[50];
    printf("Enter hunter name to reset: ");
    scanf("%s", name);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, name) == 0) {
            printf("Are you sure you want to reset hunter '%s'? (y/n): ", name);
            char confirm;
            scanf(" %c", &confirm);
            if (confirm == 'y' || confirm == 'Y') {
                sys->hunters[i].level = 1;
                sys->hunters[i].exp = 0;
                sys->hunters[i].atk = 10;
                sys->hunters[i].hp = 100;
                sys->hunters[i].def = 5;
                sys->hunters[i].banned = 0;
                printf("Hunter %s has been reset.\n", name);
            } else {
                printf("Reset cancelled.\n");
            }
            return;
        }
    }
    printf("Hunter not found.\n");
}

int main() {
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    struct SystemData *sys = shmat(shmid, NULL, 0);
    if (sys == (void *)-1) {
        perror("shmat");
        return 1;
    }

    // Inisialisasi hanya jika pertama kali
    if (sys->num_hunters == 0 && sys->num_dungeons == 0) {
        sys->num_hunters = 0;
        sys->num_dungeons = 0;
        sys->current_notification_index = 0;
    }

    srand(time(NULL));

    while (1) {
        printf("\n=== SYSTEM MENU ===\n");
        printf("1. View Hunter Info\n");
        printf("2. View Dungeon Info\n");
        printf("3. Generate Dungeon\n");
        printf("4. Ban Hunter\n");
        printf("5. Unban Hunter\n");
        printf("6. Reset Hunter\n");
        printf("7. Exit\n");
        printf("Choice: ");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1: print_hunters(sys); break;
            case 2: print_dungeons(sys); break;
            case 3: generate_dungeon(sys); break;
            case 4: ban_hunter(sys); break;
            case 5: unban_hunter(sys); break;
            case 6: reset_hunter(sys); break;
            case 7:
                shmdt(sys);
                printf("Exiting system...\n");
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }

    return 0;
}


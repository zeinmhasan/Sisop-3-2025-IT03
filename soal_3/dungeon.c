
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include "shop.c"
#define PORT 8080

// untuk menampilkan main menu
void display_menu(int client_socket)
{
    // mengirimkan text ke client
    char menu[] = 
        "\033c\033[3J"
        "\033[38;2;255;0;0m  Welcome to My Hero Academia   \e[0m\n"
        "\033[38;2;255;250;0m僕のヒーローアカデミアへようこそ\e[0m\n\n"
        "\033[38;2;105;210;82m=== Main Menu ===\e[0m\n"
        "1. Show Player Stats\n"
        "2. View Inventory\n"
        "3. Shop\n"
        "4. Equip Weapon\n"
        "5. Battle Mode\n"
        "6. About\n"
        "x. Exit\e[0m\n"
        "\033[38;2;105;210;82m=================\n"
        "Choice: \e[0m";
    send(client_socket, menu, strlen(menu), 0);
}

// inisialisasi atribut player
typedef struct
{
    int gold;
    Weapon inventory[20];
    int inventory_count;
    Weapon equipped;
    int base_damage;
    int enemies_defeated;
    int frozen;
} Player;

// senjata default "fists"
Weapon fists = {"Fists", 0, 100, "None"};

// inisialisasi enemy
typedef struct
{
    char name[50];
    int health;
    int damage;
} Enemy;

void clear_screen(int client_socket)
{
    char clear[] = "\033c\033[3J";
    send(client_socket, clear, strlen(clear), 0);
}

void trim_trailing_whitespace(char *str)
{
    int len = strlen(str);
    while(len > 0 && (str[len-1] == '\n' || str[len-1] == '\r' || str[len-1] == ' ' || str[len-1] == '\t'))
    {
        str[len-1] = '\0';
        len--;
    }
}

void display_hp_bar(int current_hp, int max_hp, char *bar, size_t bar_size)
{
    char temp[128] = {0};
    int progress = ((current_hp * 35) / max_hp);
    
    // Start red color
    strcpy(temp, "\033[1;31m[");
    
    // HP progress
    for (int i = 0; i < progress; i++)
        strcat(temp, "█");
    
    // Remaining space
    for (int i = progress; i < 35; i++)
        strcat(temp, " ");
    
    // Close bar and reset color
    strcat(temp, "]\n\033[0m");
    
    // Ensure we don't overflow
    strncpy(bar, temp, bar_size - 1);
    bar[bar_size - 1] = '\0';
}


void battle_mode(Player *player, int client_socket) 
{
    srand(time(NULL));
    int enemy_hp = rand() % 50 + 500;
    int enemy_max_hp = enemy_hp;
    char buffer[4096], hpbar[128];

    // Tampilkan musuh muncul dan bar HP awal
    snprintf(buffer, sizeof(buffer), "\033c\033[3J\033[38;2;226;79;65m============ BATTLE MODE ============\e[0m\n\nEnemy appeared! HP: %d\n", enemy_hp);
    display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
    strncat(buffer, hpbar, sizeof(buffer) - strlen(buffer) - 1);
    send(client_socket, buffer, strlen(buffer), 0);

    // Kirim prompt untuk menunggu input sebelum memasuki loop
    const char *prompt = "Enter command: '\033[38;2;105;210;82mattack\e[0m' or '\033[38;2;226;79;65mexit\e[0m' ";
    send(client_socket, prompt, strlen(prompt), 0);

    int battle_active = 1;
    while (battle_active) 
    {
        // Reset buffer untuk pesan penuh
        char full_msg[8192];
        size_t len = 0;
        full_msg[0] = '\0';

        // Baca input dari client
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, sizeof(buffer) - 1);
        if (valread <= 0) break; // Koneksi putus
        buffer[valread] = '\0';

        // Bersihkan trailing whitespace
        trim_trailing_whitespace(buffer);

        if (strcmp(buffer, "exit") == 0) 
        {
            snprintf(full_msg, sizeof(full_msg), "Exited battle mode. Press ENTER to exit.\n");
            send(client_socket, full_msg, strlen(full_msg), 0);
            battle_active = 0;
            break;
        } 
        else if (strcmp(buffer, "attack") == 0) 
        {
            if (player->frozen > 0) 
            {
                snprintf(full_msg, sizeof(full_msg), "Enemy is frozen and skips a turn!\n");
                send(client_socket, full_msg, strlen(full_msg), 0);
                player->frozen--;
                continue; // Kembali ke awal loop untuk menunggu input lagi
            }

            int dmg = player->base_damage + (rand() % 6 - 2);
            if (dmg < 0) dmg = 0;

            // Hit kritikal
            if (rand() % 10 < 2) 
            {
                dmg *= 2;
                len += snprintf(full_msg + len, sizeof(full_msg) - len, "Critical Hit!\n");
            }

            enemy_hp -= dmg;
            if (enemy_hp < 0) enemy_hp = 0;

            len += snprintf(full_msg + len, sizeof(full_msg) - len,
                "\033c\033[3J\033[38;2;226;79;65m============ BATTLE MODE ============\e[0m\n\nYou dealt %d damage! Enemy HP now: %d\n", dmg, enemy_hp);

            // Tambahkan bar HP
            display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
            len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);

            // Efek passive senjata
            if (strcmp(player->equipped.passive, "\033[38;2;158;253;209mDetroit smash\e[0m") == 0 && rand() % 100 < 30) 
            {
                int full_punch = rand() % 700;
                enemy_hp -= full_punch;
                if (enemy_hp < 0) enemy_hp = 0;
                len += snprintf(full_msg + len, sizeof(full_msg) - len,
                    "\033[38;2;158;253;209mデトロイトスマッシュ\e[0m Extra %d damage. Enemy HP now: %d\n", full_punch, enemy_hp);
                display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
            } 
            else if (strcmp(player->equipped.passive, "\033[38;5;222mAcid Sweat\e[0m") == 0 && rand() % 100 < 25) 
            {
                int acid_sweat = rand() % 500;
                enemy_hp -= acid_sweat;
                if (enemy_hp < 0) enemy_hp = 0;
                len += snprintf(full_msg + len, sizeof(full_msg) - len,
                    "\033[38;5;222mドゥアール!\e[0m Extra %d damage. Enemy HP now: %d\n", acid_sweat, enemy_hp);
                display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
            } 
            else if (strcmp(player->equipped.passive, "\033[38;5;153mFreeze\e[0m") == 0 && rand() % 100 < 20) 
            {
                player->frozen = 1;
                len += snprintf(full_msg + len, sizeof(full_msg) - len,
                    "\033[38;5;153mFreeze effect! Enemy skips next turn!\n\e[0m");
            }

            if (enemy_hp == 0) 
            {
                int gold_reward = rand() % 75 + (enemy_max_hp / 2);
                player->gold += gold_reward;   
                player->enemies_defeated++;
                len += snprintf(full_msg + len, sizeof(full_msg) - len,
                    "Enemy defeated! You gained \033[38;2;243;177;68m%d gold.\e[0m\n", gold_reward);

                enemy_hp = rand() % 100 + enemy_max_hp;
                enemy_max_hp = enemy_hp;
                len += snprintf(full_msg + len, sizeof(full_msg) - len,
                    "New Enemy appeared! HP: %d\n", enemy_hp);
                display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
            }

            // Akhiri dengan prompt supaya client tahu menunggu input
            len += snprintf(full_msg + len, sizeof(full_msg) - len, "Enter command: '\033[38;2;105;210;82mattack\e[0m' or '\033[38;2;226;79;65mexit\e[0m' ");
            send(client_socket, full_msg, len, 0);
        } 
        else 
        {
            snprintf(full_msg, sizeof(full_msg),
                "Unknown command. Use '\033[38;2;105;210;82mattack\e[0m' or '\033[38;2;226;79;65mexit\e[0m' ");
            send(client_socket, full_msg, strlen(full_msg), 0);
        }
    }
}

void handle_client(int client_socket)
{
    Player player;
    player.gold = 100;
    player.inventory_count = 0;
    player.inventory[player.inventory_count++] = fists;
    player.equipped = fists;
    player.base_damage = player.equipped.damage;
    player.enemies_defeated = 0;
    player.frozen = 0;

    char buffer[4096];
    while (1)
    {
        display_menu(client_socket);

        char buf[100] = {0};
        memset(buf, 0, sizeof(buf));
        int valread = read(client_socket, buf, sizeof(buf));
        if(valread <= 0) {
            // client disconnect
            break;
        }
        trim_trailing_whitespace(buf);

        int option = buf[0];

        switch (option)
        {
            case '1':
            {
                char msg[4096];
                snprintf(msg, sizeof(msg),
                    "\033c\033[3J"
                    "\033[38;2;105;210;82m=== PLAYER STATS ===\e[0m\n\n"
                    "\033[38;2;243;177;68mGold: %d\e[0m\n"
                    "Equipped: %s\n"
                    "\033[38;2;226;79;65mBase damage: %d\e[0m\n"
                    "\033[38;2;127;217;241mEnemies defeated: %d\e[0m\n"
                    "Passive: %s\n\n"
                    "\033[38;2;243;177;68mPress enter to return...\e[0m ",
                    player.gold, player.equipped.name,
                    player.base_damage, player.enemies_defeated,
                    player.equipped.passive);

                send(client_socket, msg, strlen(msg), 0);
                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf)); // tunggu ENTER
                break;
            }

            case '2':
            {
                char msg[4096];
                msg[0] = '\0'; // inisialisasi string kosong

                snprintf(msg, sizeof(msg), "\033c\033[3J\033[38;2;105;210;82m=== INVENTORY ===\e[0m\n\n");
                for (int i = 0; i < player.inventory_count; i++)
                {
                    char temp[100];
                    snprintf(temp, sizeof(temp), "%d. %s - \033[38;2;226;79;65mDamage: %d\e[0m - Passive: %s\n",
                        i+1, player.inventory[i].name, player.inventory[i].damage,
                        player.inventory[i].passive);
                    strncat(msg, temp, sizeof(msg) - strlen(msg) - 1);
                }
                strncat(msg, "\n\033[38;2;243;177;68mPress enter to return..\e[0m ", sizeof(msg) - strlen(msg) - 1);

                send(client_socket, msg, strlen(msg), 0);
                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf)); // tunggu ENTER
                break;
            }

            case '3':
            {
                char msg[4096];
                snprintf(msg, sizeof(msg), "\033c\033[3J\033[38;2;105;210;82mWelcome to the shop!\e[0m\n\033[38;2;243;177;68mYour gold:\e[0m %d\n\n", player.gold);
                for (int i = 0; i < total_weapons; i++)
                {
                    char weapon_msg[256];
                    snprintf (weapon_msg, sizeof(weapon_msg), "%d. %s | \033[38;2;243;177;68mPrice: %d Gold\e[0m | \033[38;2;226;79;65mDamage: %d\e[0m | Passive: %s\n",
                        i + 1, weapons[i].name, weapons[i].price, weapons[i].damage, weapons[i].passive);
                    strncat(msg, weapon_msg, sizeof(msg) - strlen(msg) - 1);
                }
                strncat(msg, "\n\033[38;2;243;177;68mEnter the weapon you want to purchase (or 0 to cancel): \e[0m", sizeof(msg) - strlen(msg) - 1);

                send(client_socket, msg, strlen(msg), 0);

                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf));
                if(valread <= 0) break; // client disconnect

                int choice = atoi(buf);
                if (choice == 0) break;

                Weapon selected = buy_weapon(choice);

                if (strlen(selected.name) == 0)
                {
                    const char error[] = "\033[38;2;226;79;65mInvalid choice. Press enter to return...\e[0m ";
                    send(client_socket, error, strlen(error), 0);
                    memset(buf, 0, sizeof(buf));
                    valread = read(client_socket, buf, sizeof(buf));
                    break;
                }

                if (player.gold >= selected.price)
                {
                    player.gold -= selected.price;
                    player.inventory[player.inventory_count++] = selected;

                    char success[256];
                    snprintf(success, sizeof(success),
                        "\033[38;2;243;177;68mSuccessfully purchased %s for %d gold!\nPress enter to return...\e[0m ",
                        selected.name, selected.price);
                    send(client_socket, success, strlen(success), 0);
                }
                else
                {
                    const char failed[] = "\033[38;2;226;79;65mNot enough gold! Press enter to return...\e[0m ";
                    send(client_socket, failed, strlen(failed), 0);
                }
                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf)); // tunggu ENTER
                break;
            }

            case '4':
            {
                char msg[4096];
                msg[0] = '\0';

                if (player.inventory_count == 0)
                {
                    const char empty_inv[] = "\033[38;2;226;79;65mInventory is empty. Press enter to return..\e[0m ";
                    send(client_socket, empty_inv, strlen(empty_inv), 0);
                    memset(buf, 0, sizeof(buf));
                    valread = read(client_socket, buf, sizeof(buf));
                    break;
                }

                snprintf(msg, sizeof(msg), "\033c\033[3J\033[38;2;105;210;82mPlease choose your weapon\e[0m\n\n");
                for (int i = 0; i < player.inventory_count; i++)
                {
                    char temp[200];
                    snprintf(temp, sizeof(temp), "%d. %s - \033[38;2;226;79;65mDamage: %d\e[0m - Passive: %s\n",
                        i+1, player.inventory[i].name, player.inventory[i].damage,
                        player.inventory[i].passive);
                    strncat(msg, temp, sizeof(msg) - strlen(msg) - 1);
                }
                strncat(msg, "\n\033[38;2;243;177;68mSelect a weapon number to use (or press 0 to cancel): \e[0m", sizeof(msg) - strlen(msg) - 1);

                send(client_socket, msg, strlen(msg), 0);

                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf));
                if (valread <= 0) break;

                int choice = atoi(buf);
                if(choice > 0 && choice <= player.inventory_count)
                {
                    player.equipped = player.inventory[choice - 1];
                    player.base_damage = player.equipped.damage;

                    char confirm[128];
                    snprintf(confirm, sizeof(confirm), "\033[38;2;243;177;68mWeapon %s berhasil digunakan.\nPress enter to return...\e[0m ", player.equipped.name);
                    send(client_socket, confirm, strlen(confirm), 0);
                }
                else
                {
                    const char cancel[] = "\033[38;2;243;177;68mCancel weapon selection.\nPress enter to return...\e[0m ";
                    send(client_socket, cancel, strlen(cancel), 0);
                }

                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf));
                break;
            }

            case '5':
            {
                battle_mode(&player, client_socket);
                char dummy[100] = {0};
                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, dummy, sizeof(dummy));
                break;
            }

            case '6':
            {
                const char msg[] = "\033c\033[3J"
                             "\033[38;2;158;253;209m\033[38;2;255;0;0m====== My Hero Academia ======\e[0m\n\033[38;2;255;250;0m=== 僕のヒーローアカデミア ===\e[0m\n\n"
                             "Members:\n"
                             "1. Zein Muhammad Hasan (5027241035)\n"
                             "2. Afriza Tristan Calendra Rajasa (5027241104)\n"
                             "3. Jofanka Al-Kautsar Pangestu Abady (5027241107)\n\n"
                             "\033[38;2;243;177;68mPress enter to return...\e[0m  ";
                send(client_socket, msg, strlen(msg), 0);
                memset(buf, 0, sizeof(buf));
                valread = read(client_socket, buf, sizeof(buf));
                break;
            }

            case 'x':
            {
                printf("Player has disconnected to the server.\n");
                close(client_socket);
                return;
            }

            default:
            {
                const char msg[] = "\033[38;2;226;79;65mInvalid input. Press enter to return... \e[0m";
                send(client_socket, msg, strlen(msg), 0);
                memset(buffer, 0, sizeof(buffer));
                valread = read(client_socket, buf, sizeof(buf));
                break;
            }
        }
    }
}

int main()
{
    int new_socket;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perror("Socket failed");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(addr);
    
    if(bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("Bind failed");
        return -1;
    }

    if (listen(sfd, 1) == -1)
    {
        perror("Listen failed");
        return -1;
    }

    printf("The server is up on port %d\n", PORT);

    while(1)
    {
        new_socket = accept(sfd, (struct sockaddr *)&addr, &addrlen);
        if (new_socket < 0)
        {
            perror("Accept");
            return -1;
        }

        printf("Player has connected to the server.\n");

        pid_t pid = fork();
        if(pid == 0)
        {
            close(sfd);
            handle_client(new_socket);
            exit(0);
        }
        else if (pid < 0)
        {
            perror("Fork failed");
        }
        close(new_socket);
    }
}

// shop.c
#include <stdio.h>
#include <string.h>

typedef struct 
{
    char name[100];
    int price;
    int damage;
    char passive[50];
} Weapon;

Weapon weapons[] = 
{
    {"\033[38;5;203m硬化 (hardening)\e[0m", 400, 200, "None"},
    {"\033[38;5;98m黒影 (dark Shadow)\e[0m", 650, 350, "None"},
    {"\033[38;5;153m半冷半燃\e[0m \033[38;5;209m(Half-cool, half-burn)\e[0m", 750, 450, "\033[38;5;153mFreeze\e[0m"},
    {"\033[38;5;222m爆破 (Exploison)\e[0m", 850, 475, "\033[38;5;222mAcid Sweat\e[0m"},
    {"\033[38;2;158;253;209mワン・フォー・オール (One For All)\e[0m", 1000, 500, "\033[38;2;158;253;209mDetroit smash\e[0m"}
};

int total_weapons = 5;

void show_shop() 
{
    printf("\n=== Weapon Shop ===\n");
    for (int i = 0; i < total_weapons; i++) {
        printf("%d. %s - \033[38;2;243;177;68mPrice: %d Gold\e[0m - Damage: %d - Passive: %s\n",
               i+1, weapons[i].name, weapons[i].price, weapons[i].damage, weapons[i].passive);
    }
    printf("===================\n");
}

Weapon buy_weapon(int choice) 
{
    if (choice < 1 || choice > total_weapons) 
    {
        Weapon empty = {"", 0, 0, ""};
        return empty;
    }
    return weapons[choice-1];
}


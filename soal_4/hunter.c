#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include "shm_common.h"

struct SystemData *system_data;
int shm_id;
int hunter_index = -1;

void notify_dungeons() {
    while (1) {
        system("clear");
        printf("\n=== Dungeon Notifications ===\n");
        int shown = 0;
        for (int i = 0; i < system_data->num_dungeons; ++i) {
            if (system_data->hunters[hunter_index].level >= system_data->dungeons[i].min_level) {
                struct Dungeon d = system_data->dungeons[i];
                printf("[%d] %s (Min Lvl: %d) => EXP+%d ATK+%d HP+%d DEF+%d\n", i+1,
                    d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
                shown++;
            }
        }
        if (shown == 0) printf("No dungeons available for your level.\n");
        sleep(3);
    }
}

void register_hunter() {
    if (system_data->num_hunters >= MAX_HUNTERS) {
        printf("Hunter list full!\n");
        return;
    }
    char name[50];
    printf("Enter username to register: ");
    scanf("%s", name);

    for (int i = 0; i < system_data->num_hunters; ++i) {
        if (strcmp(system_data->hunters[i].username, name) == 0) {
            printf("Username already taken!\n");
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
    h.shm_key = rand() % 10000 + 1000;

    system_data->hunters[system_data->num_hunters] = h;
    hunter_index = system_data->num_hunters;
    system_data->num_hunters++;
    printf("Registration successful. Welcome, %s!\n", name);
}

int login() {
    char name[50];
    printf("Enter username to login: ");
    scanf("%s", name);
    for (int i = 0; i < system_data->num_hunters; ++i) {
        if (strcmp(system_data->hunters[i].username, name) == 0) {
            hunter_index = i;
            printf("Login successful. Welcome back, %s!\n", name);
            return 1;
        }
    }
    printf("Hunter not found!\n");
    return 0;
}

void view_dungeons() {
    printf("\n=== Available Dungeons ===\n");
    for (int i = 0; i < system_data->num_dungeons; ++i) {
        if (system_data->hunters[hunter_index].level >= system_data->dungeons[i].min_level) {
            struct Dungeon d = system_data->dungeons[i];
            printf("[%d] %s | EXP+%d ATK+%d HP+%d DEF+%d\n", i, d.name, d.exp, d.atk, d.hp, d.def);
        }
    }
}

void raid_dungeon() {
    view_dungeons();
    printf("Enter dungeon index to raid: ");
    int index;
    scanf("%d", &index);

    if (index < 0 || index >= system_data->num_dungeons) {
        printf("Invalid index!\n");
        return;
    }

    struct Hunter *h = &system_data->hunters[hunter_index];
    struct Dungeon d = system_data->dungeons[index];

    if (h->level < d.min_level) {
        printf("Your level is too low to raid this dungeon.\n");
        return;
    }
    if (h->banned) {
        printf("You are banned from raiding.\n");
        return;
    }

    h->exp += d.exp;
    h->atk += d.atk;
    h->hp += d.hp;
    h->def += d.def;

    if (h->exp >= 500) {
        h->level++;
        h->exp = 0;
        printf("Congratulations! You leveled up!\n");
    }

    for (int i = index; i < system_data->num_dungeons - 1; ++i) {
        system_data->dungeons[i] = system_data->dungeons[i + 1];
    }
    system_data->num_dungeons--;
    printf("Dungeon conquered successfully!\n");
}

void battle_hunter() {
    char target[50];
    printf("Enter opponent hunter username: ");
    scanf("%s", target);

    struct Hunter *me = &system_data->hunters[hunter_index];
    struct Hunter *enemy = NULL;
    int enemy_index = -1;

    for (int i = 0; i < system_data->num_hunters; ++i) {
        if (i != hunter_index && strcmp(system_data->hunters[i].username, target) == 0) {
            enemy = &system_data->hunters[i];
            enemy_index = i;
            break;
        }
    }

    if (!enemy) {
        printf("Opponent not found!\n");
        return;
    }
    if (me->banned) {
        printf("You are banned from battling.\n");
        return;
    }

    int my_power = me->atk + me->hp + me->def;
    int enemy_power = enemy->atk + enemy->hp + enemy->def;

    if (my_power >= enemy_power) {
        me->atk += enemy->atk;
        me->hp += enemy->hp;
        me->def += enemy->def;
        for (int i = enemy_index; i < system_data->num_hunters - 1; ++i) {
            system_data->hunters[i] = system_data->hunters[i + 1];
        }
        system_data->num_hunters--;
        printf("You won the battle and absorbed your opponent's stats!\n");
    } else {
        enemy->atk += me->atk;
        enemy->hp += me->hp;
        enemy->def += me->def;
        for (int i = hunter_index; i < system_data->num_hunters - 1; ++i) {
            system_data->hunters[i] = system_data->hunters[i + 1];
        }
        system_data->num_hunters--;
        printf("You lost the battle and have been eliminated...\n");
        exit(0);
    }
}

int main() {
    key_t key = get_system_key(); // 🔧 Menggunakan key dari shm_common.h

    shm_id = shmget(key, sizeof(struct SystemData), 0666);
    if (shm_id == -1) {
        perror("shm_common.h");
        exit(EXIT_FAILURE);
    }

    system_data = (struct SystemData *)shmat(shm_id, NULL, 0);
    if (system_data == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    int option;
    while (1) {
        printf("\n=== Hunter Menu ===\n");
        printf("1. Register\n2. Login\n3. Exit\nChoose: ");
        scanf("%d", &option);
        if (option == 1) register_hunter();
        else if (option == 2 && login()) break;
        else if (option == 3) exit(0);
    }

    while (1) {
        printf("\n=== Hunter Actions ===\n");
        printf("1. View Dungeons\n2. Raid Dungeon\n3. Battle Hunter\n4. Start Dungeon Notifications\n5. Exit\nChoose: ");
        scanf("%d", &option);
        if (option == 1) view_dungeons();
        else if (option == 2) raid_dungeon();
        else if (option == 3) battle_hunter();
        else if (option == 4) {
            pthread_t notif_thread;
            pthread_create(&notif_thread, NULL, (void *)notify_dungeons, NULL);
            pthread_join(notif_thread, NULL);
        }
        else if (option == 5) exit(0);
    }

    return 0;
}


#include "shm_common.h"

struct SystemData *system_data;
int shm_id;

void handle_exit(int sig) {
    printf("\nCleaning up shared memory...\n");
    if (shmdt(system_data) == -1) {
        perror("shmdt");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
    printf("Shared memory cleaned up. Exiting.\n");
    exit(0);
}

void list_hunters() {
    printf("\n=== List of Hunters ===\n");
    for (int i = 0; i < system_data->num_hunters; ++i) {
        struct Hunter h = system_data->hunters[i];
        printf("Name: %s | Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d | Status: %s\n",
               h.username, h.level, h.exp, h.atk, h.hp, h.def, h.banned ? "BANNED" : "ACTIVE");
    }
}

void list_dungeons() {
    printf("\n=== List of Dungeons ===\n");
    for (int i = 0; i < system_data->num_dungeons; ++i) {
        struct Dungeon d = system_data->dungeons[i];
        printf("Name: %s | Min Level: %d | Reward: EXP+%d ATK+%d HP+%d DEF+%d\n",
               d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
    }
}

void add_dungeon() {
    if (system_data->num_dungeons >= MAX_DUNGEONS) {
        printf("Dungeon list full!\n");
        return;
    }

    char *names[] = {"Ancient Ruins", "Dragon's Lair", "Cursed Forest", "Demon's Castle", "Mystic Cave"};
    srand(time(NULL));

    struct Dungeon d;
    strcpy(d.name, names[rand() % 5]);
    d.min_level = rand() % 5 + 1;
    d.exp = rand() % 151 + 150;
    d.atk = rand() % 51 + 100;
    d.hp = rand() % 51 + 50;
    d.def = rand() % 26 + 25;
    d.shm_key = rand() % 10000 + 1000;

    system_data->dungeons[system_data->num_dungeons++] = d;
    printf("Dungeon '%s' added!\n", d.name);
}

void ban_hunter() {
    char name[50];
    printf("Enter hunter name to ban: ");
    scanf("%s", name);
    for (int i = 0; i < system_data->num_hunters; ++i) {
        if (strcmp(system_data->hunters[i].username, name) == 0) {
            system_data->hunters[i].banned = 1;
            printf("Hunter %s has been banned.\n", name);
            return;
        }
    }
    printf("Hunter not found!\n");
}

void unban_hunter() {
    char name[50];
    printf("Enter hunter name to unban: ");
    scanf("%s", name);
    for (int i = 0; i < system_data->num_hunters; ++i) {
        if (strcmp(system_data->hunters[i].username, name) == 0) {
            system_data->hunters[i].banned = 0;
            printf("Hunter %s has been unbanned.\n", name);
            return;
        }
    }
    printf("Hunter not found!\n");
}

void reset_hunter() {
    char name[50];
    printf("Enter hunter name to reset: ");
    scanf("%s", name);
    for (int i = 0; i < system_data->num_hunters; ++i) {
        if (strcmp(system_data->hunters[i].username, name) == 0) {
            system_data->hunters[i].level = 1;
            system_data->hunters[i].exp = 0;
            system_data->hunters[i].atk = 10;
            system_data->hunters[i].hp = 100;
            system_data->hunters[i].def = 5;
            system_data->hunters[i].banned = 0;
            printf("Hunter %s has been reset.\n", name);
            return;
        }
    }
    printf("Hunter not found!\n");
}

int main() {
    signal(SIGINT, handle_exit);

    key_t key = get_system_key();
    shm_id = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    system_data = (struct SystemData *)shmat(shm_id, NULL, 0);
    if (system_data == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    printf("System is running...\n");

    int choice;
    while (1) {
        printf("\n=== System Menu ===\n");
        printf("1. List Hunters\n");
        printf("2. List Dungeons\n");
        printf("3. Add Dungeon\n");
        printf("4. Ban Hunter\n");
        printf("5. Unban Hunter\n");
        printf("6. Reset Hunter\n");
        printf("7. Exit\n");
        printf("Choose: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: list_hunters(); break;
            case 2: list_dungeons(); break;
            case 3: add_dungeon(); break;
            case 4: ban_hunter(); break;
            case 5: unban_hunter(); break;
            case 6: reset_hunter(); break;
            case 7: handle_exit(SIGINT); break;
            default: printf("Invalid choice.\n");
        }
    }

    return 0;
}

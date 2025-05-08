#Nomer 2

Di soal nomer 2, kita diminta membuat sebuah Delivery Management System untuk perusahaan ekspedisi RushGo
a. Buat file nano delivery_agent
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define SHM_SIZE 4096
#define MAX_ORDERS 50

typedef struct {
    char name[50];
    char address[100];
    char type[20];
    char status[20];
    char agent[20];
} Order;

Order *orders;
int order_count = 0;
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_delivery(const char *agent, const char *type, const Order *order) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", tm);
    
    FILE *log_file = fopen("delivery.log", "a");
    if (!log_file) {
        perror("Failed to open log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    fprintf(log_file, "[%s] [AGENT %s] %s package delivered to %s in %s\n",
            timestamp, agent, type, order->name, order->address);
    
    fclose(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void load_orders_from_shm() {
    pthread_mutex_lock(&shm_mutex);
    
    key_t key = ftok("delivery_ref", 65);
    int shmid = shmget(key, SHM_SIZE, 0666|IPC_CREAT);
    Order *shm_orders = (Order*)shmat(shmid, NULL, 0);

    order_count = 0;
    while (order_count < MAX_ORDERS && strlen(shm_orders[order_count].name) > 0) {
        order_count++;
    }

    orders = realloc(orders, order_count * sizeof(Order));
    memcpy(orders, shm_orders, order_count * sizeof(Order));

    shmdt(shm_orders);
    pthread_mutex_unlock(&shm_mutex);
}

void *agent_work(void *arg) {
    char agent_name = *(char *)arg;
    
    while (1) {
        load_orders_from_shm();
        
        for (int i = 0; i < order_count; i++) {
            if (strcmp(orders[i].type, "Express") == 0 && 
                strcmp(orders[i].status, "Pending") == 0) {
                
                pthread_mutex_lock(&shm_mutex);
                key_t key = ftok("delivery_ref", 65);
                int shmid = shmget(key, SHM_SIZE, 0666|IPC_CREAT);
                Order *shm_orders = (Order*)shmat(shmid, NULL, 0);
                
                strcpy(shm_orders[i].status, "Delivered");
                strcpy(shm_orders[i].agent, &agent_name);
                
                log_delivery(&agent_name, "Express", &orders[i]);
                
                shmdt(shm_orders);
                pthread_mutex_unlock(&shm_mutex);
                
                sleep(2); // Simulate delivery time
            }
        }
        sleep(5);
    }
    return NULL;
}

int main() {
    pthread_t agent_a, agent_b, agent_c;
    char a = 'A', b = 'B', c = 'C';

    pthread_create(&agent_a, NULL, agent_work, &a);
    pthread_create(&agent_b, NULL, agent_work, &b);
    pthread_create(&agent_c, NULL, agent_work, &c);

    pthread_join(agent_a, NULL);
    pthread_join(agent_b, NULL);
    pthread_join(agent_c, NULL);

    free(orders);
    return 0;
}
```

b. Buat file nano dispatcher.c
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define SHM_SIZE 4096
#define MAX_ORDERS 50

typedef struct {
    char name[50];
    char address[100];
    char type[20];
    char status[20];
    char agent[20];
} Order;

Order *orders;
int order_count = 0;
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_delivery(const char *agent, const char *type, const Order *order) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", tm);
    
    FILE *log_file = fopen("delivery.log", "a");
    if (!log_file) {
        perror("Failed to open log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    fprintf(log_file, "[%s] [AGENT %s] %s package delivered to %s in %s\n",
            timestamp, agent, type, order->name, order->address);
    
    fclose(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void load_orders_from_shm() {
    pthread_mutex_lock(&shm_mutex);
    
    key_t key = ftok("delivery_ref", 65);
    int shmid = shmget(key, SHM_SIZE, 0666|IPC_CREAT);
    Order *shm_orders = (Order*)shmat(shmid, NULL, 0);

    order_count = 0;
    while (order_count < MAX_ORDERS && strlen(shm_orders[order_count].name) > 0) {
        order_count++;
    }

    orders = realloc(orders, order_count * sizeof(Order));
    memcpy(orders, shm_orders, order_count * sizeof(Order));

    shmdt(shm_orders);
    pthread_mutex_unlock(&shm_mutex);
}

void deliver_reguler(const char *name, const char *user) {
    load_orders_from_shm();
    
    for (int i = 0; i < order_count; i++) {
        if (strcmp(orders[i].name, name) == 0 && 
            strcmp(orders[i].type, "Reguler") == 0 &&
            strcmp(orders[i].status, "Pending") == 0) {
            
            pthread_mutex_lock(&shm_mutex);
            key_t key = ftok("delivery_ref", 65);
            int shmid = shmget(key, SHM_SIZE, 0666|IPC_CREAT);
            Order *shm_orders = (Order*)shmat(shmid, NULL, 0);
            
            strcpy(shm_orders[i].status, "Delivered");
            strcpy(shm_orders[i].agent, user);
            
            log_delivery(user, "Reguler", &orders[i]);
            
            shmdt(shm_orders);
            pthread_mutex_unlock(&shm_mutex);
            
            printf("Package delivered successfully!\n");
            return;
        }
    }
    printf("No pending Reguler order found for %s\n", name);
}

void check_status(const char *name) {
    load_orders_from_shm();
    
    for (int i = 0; i < order_count; i++) {
        if (strcmp(orders[i].name, name) == 0) {
            if (strcmp(orders[i].status, "Delivered") == 0) {
                printf("Status for %s: Delivered by Agent %s\n", 
                       orders[i].name, orders[i].agent);
            } else {
                printf("Status for %s: Pending\n", orders[i].name);
            }
            return;
        }
    }
    printf("No order found for %s\n", name);
}

void list_orders() {
    load_orders_from_shm();
    printf("%-20s %-20s %-15s %-15s %-10s\n", 
           "Name", "Address", "Type", "Status", "Agent");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < order_count; i++) {
        printf("%-20s %-20s %-15s %-15s %-10s\n", 
               orders[i].name, orders[i].address, 
               orders[i].type, orders[i].status, orders[i].agent);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  ./dispatcher -deliver [Name]\n");
        printf("  ./dispatcher -status [Name]\n");
        printf("  ./dispatcher -list\n");
        return 1;
    }

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        deliver_reguler(argv[2], getenv("USER"));
    } 
    else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        check_status(argv[2]);
    }
    else if (strcmp(argv[1], "-list") == 0 && argc == 2) {
        list_orders();
    } 
    else {
        printf("Invalid command or arguments\n");
        return 1;
    }

    free(orders);
    return 0;
}
```

c. Fitur untuk mengecek deliver.log untuk melihat catatan pengiriman

d. Untuk pengumuman reguler, menggunakan ./dispatcher -deliver [nama[

e. Untuk mengecek status pesanan, menggunakan ./dispatcher -status [nama]

f. Untuk melihat semua order disertai nama dan status, menggunakan ./dispatcher -list

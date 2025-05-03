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
        printf("  ./dispatcher -list\n");
        return 1;
    }

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        deliver_reguler(argv[2], getenv("USER"));
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

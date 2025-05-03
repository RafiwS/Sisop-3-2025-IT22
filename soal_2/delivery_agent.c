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

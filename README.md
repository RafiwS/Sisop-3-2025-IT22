#Nomor 1

Di soal ini, kita diminta untuk membuat sistem client-server berbasis socket RPC di C, di mana client dapat mengirim file teks terenkripsi ke server untuk didekripsi dan disimpan sebagai file JPEG, serta mengunduh file JPEG dari server.

1. image_server
   a. build_path
   ```
   void build_path(char *dest, const char *subpath) {
    snprintf(dest, PATH_MAX, "%s/%s", base_dir, subpath);
    }
   ```
   Tujuan: Menyusun path absolut ke file/subdirektori dalam server.

   b. write_log
   ```
   void write_log(const char *action, const char *info) {
    char log_path[PATH_MAX];
    build_path(log_path, "server.log");

    FILE *log = fopen(log_path, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", t);
    fprintf(log, "[Server]%s: [%s] [%s]\n", buf, action, info);
    fclose(log);
    }
   ```
   Tujuan: Menuliskan log aktivitas server ke server.log.

   c. reverse_string
   ```
   void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
        }
    }
   ```
   Tujuan: Membalik isi string (untuk proses dekripsi).

   d. hex_to_bytes
   ```
   int hex_to_bytes(const char *hex, unsigned char *out) {
    int len = strlen(hex);
    for (int i = 0; i < len / 2; i++) {
        sscanf(hex + 2 * i, "%2hhx", &out[i]);
        }
    return len / 2;
    }
   ```
   Tujuan: Mengonversi string hex ke bentuk biner.

   e. handle_upload
   ```
   void handle_upload(int client_sock, char *data) {
    reverse_string(data);

    unsigned char binary[BUF_SIZE];
    int bin_len = hex_to_bytes(data, binary);

    time_t t = time(NULL);
    char img_path[PATH_MAX];
    char filename[64];
    sprintf(filename, "%ld.jpeg", t);
    build_path(img_path, "database/");
    strcat(img_path, filename);

    FILE *img = fopen(img_path, "wb");
    if (!img) {
        send(client_sock, "ERROR: Cannot save image", 25, 0);
        return;
        }

    fwrite(binary, 1, bin_len, img);
    fclose(img);

    write_log("SAVE", filename);
    send(client_sock, "Saved and decrypted", 19, 0);
    }
   ```
   Tujuan: Menerima data terenkripsi dari client, lalu decode dan simpan sebagai file JPEG.

   f. handle_download
   ```
   void handle_download(int client_sock, const char *filename) {
    char path[PATH_MAX];
    build_path(path, "database/");
    strcat(path, filename);
    
    FILE *f = fopen(path, "rb");
    if (!f) {
        send(client_sock, "ERROR: File not found", 22, 0);
        return;
        }

    char buffer[BUF_SIZE];
    int bytes;
    while ((bytes = fread(buffer, 1, BUF_SIZE, f)) > 0) {
        send(client_sock, buffer, bytes, 0);
        }

    fclose(f);
    write_log("DOWNLOAD", filename);
    }
   ```
   Tujuan: Mengirim file JPEG ke client sesuai permintaan filename.

   g. main
   ```
   int main() {
    if (getcwd(base_dir, sizeof(base_dir)) == NULL) {
        perror("getcwd failed");
        exit(EXIT_FAILURE);
        }

    pid_t pid = fork();
    if (pid != 0) return 0; // daemonize

    int server_fd, client_sock;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    while (1) {
        client_sock = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_sock < 0) continue;

        char buffer[BUF_SIZE];
        int valread = recv(client_sock, buffer, BUF_SIZE - 1, 0);
        if (valread <= 0) {
            close(client_sock);
            continue;
            }
        buffer[valread] = '\0';

        if (strncmp(buffer, "UPLOAD ", 7) == 0) {
            write_log("UPLOAD", buffer + 7);
            char content[BUF_SIZE];
            valread = recv(client_sock, content, BUF_SIZE - 1, 0);
            if (valread > 0) {
                content[valread] = '\0';
                handle_upload(client_sock, content);
            }
            } else if (strncmp(buffer, "DOWNLOAD ", 9) == 0) {
            handle_download(client_sock, buffer + 9);
            } else if (strncmp(buffer, "EXIT", 4) == 0) {
            write_log("EXIT", "Client requested to exit");
            }

        close(client_sock);
        }

    return 0;
    }
   ```
Fungsi main() pada program image_server.c bertanggung jawab untuk menjalankan server secara daemon dan memproses permintaan client melalui koneksi TCP. Pertama, program menyimpan direktori kerja saat ini menggunakan getcwd() ke dalam variabel global base_dir, yang nantinya dipakai untuk membangun path absolut file log dan database. Kemudian, dengan fork(), server menciptakan proses anak dan segera mengakhiri proses induk untuk menjalankan daemon di latar belakang. Setelah menjadi daemon, server membuat socket TCP (SOCK_STREAM), mengikatnya ke semua alamat IP (INADDR_ANY) di port 9090, dan mulai mendengarkan koneksi masuk. Di dalam loop tak hingga, server menerima koneksi client satu per satu menggunakan accept(). Setelah menerima koneksi, server membaca perintah dari client menggunakan recv(). Bila perintah adalah UPLOAD, server akan menerima data file terenkripsi, mendekripsinya, menyimpannya sebagai file JPEG dengan nama berdasarkan timestamp, dan mencatat log. Bila perintah adalah DOWNLOAD, server membaca file yang diminta dari database dan mengirimkannya ke client. Jika perintah adalah EXIT, server hanya mencatat permintaan keluar. Setelah setiap sesi client selesai, socket ditutup dan server kembali siap menerima koneksi baru.

2. image_client
   a. upload_file
   ```
   void upload_file(int sock, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("File not found!\n");
        return;
        }

    char msg[256];
    sprintf(msg, "UPLOAD %s", filename);
    send(sock, msg, strlen(msg), 0);
    sleep(1);

    char buffer[BUF_SIZE];
    int bytes;
    while ((bytes = fread(buffer, 1, BUF_SIZE, f)) > 0) {
        send(sock, buffer, bytes, 0);
        }
    fclose(f);

    bytes = recv(sock, buffer, BUF_SIZE, 0);
    buffer[bytes] = '\0';
    printf("Server: %s\n", buffer);
    }
   ```
   Fungsi ini bertugas mengirimkan file teks ke server untuk didekripsi.
- Membuka file teks yang ada di direktori secrets/.
- Mengirim perintah "UPLOAD <filename>" ke server.
- Menunggu 1 detik agar server siap menerima isi file.
- Mengirimkan isi file dalam blok/blok buffer ke server.
- Menerima respon dari server setelah file berhasil diproses.

    b. download_file
    ```
    void download_file(int sock, const char *filename) {
    char msg[256];
    sprintf(msg, "DOWNLOAD %s", filename);
    send(sock, msg, strlen(msg), 0);

    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Gagal buka file output\n");
        return;
        }

    char buffer[BUF_SIZE];
    int bytes;
    while ((bytes = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes, f);
        if (bytes < BUF_SIZE) break;
        }

    fclose(f);
    }  
    ```
    Fungsi ini meminta file JPEG dari server berdasarkan nama file (timestamp).
- Mengirimkan perintah "DOWNLOAD <filename>" ke server.
- Membuka (atau membuat) file lokal untuk menampung hasil unduhan.
- Menerima data secara bertahap dari socket.
- Menulis data ke file hingga semua data diterima.

     c. main
    ``` 
    while (1) {
        printf("| Image Decoder Client |\n");
        printf("1. Send input file to server\n");
        printf("2. Download file from server\n");
        printf("3. Exit\n>> ");

        int choice;
        scanf("%d", &choice);
        getchar();

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server = {
            .sin_family = AF_INET,
            .sin_port = htons(PORT),
            };
        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            perror("Connect failed");
            close(sock);
            continue;
            }
        if (choice == 3){
                char msg[256];
                sprintf(msg, "EXIT");
                send(sock, msg, strlen(msg), 0);
                break;
            }

        char fname[256];
        printf("Enter the file name: ");
        scanf("%s", fname);

        if (choice == 1) {
            char path[300];
            sprintf(path, "secrets/%s", fname);
            upload_file(sock, path);
        } else if (choice == 2) {
            download_file(sock, fname);
            }

        close(sock);
        }

    return 0;
    }
    ```
    Fungsi utama ini menampilkan menu interaktif kepada pengguna.
- User memilih apakah ingin mengirim file, mengunduh file, atau keluar.
- Setiap iterasi membuat koneksi TCP baru ke server (127.0.0.1:9090).
- Jika opsi 1 dipilih, file akan dibaca dari folder secrets/ lalu dikirim ke server.
- Jika opsi 2, client meminta file JPEG berdasarkan nama file.
- Jika memilih Exit (3), client mengirim "EXIT" ke server dan keluar dari program.


#Nomor 2

Di soal nomor 2, kita diminta membuat sebuah Delivery Management System untuk perusahaan ekspedisi RushGo
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

#soal no 4
pada soal ini kita disuruh membuat code dengan clue dari shm_commmon.sh yang sudah diberikan di soal, pada soal ini saya menggunakan shared memory untuk menyimpan data sesuai dengan system.c dan hunter.c

```c
#include "shm_common.h"
#include <errno.h>

struct SystemData *system_data;
int shm_id;

void handle_exit(int sig) {
    if (shmdt(system_data) == -1) {
        perror("shmdt");
    }
    printf("Shared memory detached. Exiting.\n");
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

    char *names[] = {
        "Double Dungeon", "Demon Castle", "Pyramid Dungeon", "Red Gate Dungeon",
        "Hunters Guild Dungeon", "Busan A-Rank Dungeon", "Insects Dungeon",
        "Goblins Dungeon", "D-Rank Dungeon", "Gwanak Mountain Dungeon",
        "Hapjeong Subway Station Dungeon"
    };

    srand(time(NULL));
    struct Dungeon d;
    strcpy(d.name, names[rand() % 11]);
    d.min_level = rand() % 5 + 1;
    d.exp = rand() % 151 + 150;
    d.atk = rand() % 51 + 100;
    d.hp = rand() % 51 + 50;
    d.def = rand() % 26 + 25;

    system_data->dungeons[system_data->num_dungeons++] = d;
    printf("Dungeon '%s' added!\n", d.name);
}

void ban_hunter() {
    char name[50];
    printf("Enter hunter name to ban: ");
    if (scanf("%49s", name) != 1) {
        printf("Invalid input.\n");
        return;
    }
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
    if (scanf("%49s", name) != 1) {
        printf("Invalid input.\n");
        return;
    }
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
    if (scanf("%49s", name) != 1) {
        printf("Invalid input.\n");
        return;
    }
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
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            while (getchar() != '\n');
            continue;
        }

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
```
code di atas merupakan code dari system.c, di mana code ini menampilkan bagian dari system yang akan bisa melihat status hunter dan juga dungeon
output yang diberikan : 


![Image](https://github.com/user-attachments/assets/8415ec1b-fa49-405b-9cf9-9825da3ded0f)


selanjutnya ada hunter.c di mana pada code hunter.c ini menyimpan dan menambahkan user hunter sebagai salah satu pemain, di hunter.c ini sebagai hunter kita akan dimulai dari level 1

```c
#include "shm_common.h"

struct SystemData *shared_data;
int shmid;
int logged_in_index = -1;

void connect_shared_memory() {
    key_t key = get_system_key();
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Gunakan IPC_CREAT untuk memastikan shared memory dapat dibuat jika belum ada
    shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        printf("Gagal mengakses shared memory. Pastikan system.c sudah dijalankan.\n");
        exit(1);
    }

    shared_data = shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }
}

void register_hunter() {
    char username[50];
    printf("Masukkan username: ");
    scanf("%s", username);

    for (int i = 0; i < shared_data->num_hunters; i++) {
        if (strcmp(shared_data->hunters[i].username, username) == 0) {
            printf("Username sudah digunakan.\n");
            return;
        }
    }

    if (shared_data->num_hunters >= MAX_HUNTERS) {
        printf("Jumlah hunter sudah maksimum.\n");
        return;
    }

    struct Hunter new_hunter;
    strcpy(new_hunter.username, username);
    new_hunter.level = 1;
    new_hunter.exp = 0;
    new_hunter.atk = 10;
    new_hunter.hp = 100;
    new_hunter.def = 5;
    new_hunter.banned = 0;

    shared_data->hunters[shared_data->num_hunters] = new_hunter;
    shared_data->num_hunters++;

    printf("Hunter berhasil didaftarkan.\n");
}

void login_hunter() {
    char username[50];
    printf("Masukkan username: ");
    scanf("%s", username);

    for (int i = 0; i < shared_data->num_hunters; i++) {
        if (strcmp(shared_data->hunters[i].username, username) == 0) {
            if (shared_data->hunters[i].banned) {
                printf("Hunter ini sedang di-ban.\n");
                return;
            }
            logged_in_index = i;
            printf("Login berhasil sebagai %s.\n", username);
            return;
        }
    }

    printf("Username tidak ditemukan.\n");
}

void tampilkan_dungeon() {
    printf("\n=== Daftar Dungeon yang Dapat Dimasuki ===\n");
    for (int i = 0; i < shared_data->num_dungeons; i++) {
        struct Dungeon d = shared_data->dungeons[i];
        if (shared_data->hunters[logged_in_index].level >= d.min_level) {
            printf("- %s (min level %d, exp %d, atk %d, hp %d, def %d)\n",
                   d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
        }
    }
}

void menu() {
    int pilihan;
    do {
        printf("\n=== Menu Hunter ===\n");
        printf("1. Register Hunter\n");
        printf("2. Login Hunter\n");
        printf("3. Tampilkan Dungeon\n");
        printf("0. Exit\n");
        printf("Pilih: ");
        scanf("%d", &pilihan);

        switch (pilihan) {
            case 1:
                register_hunter();
                break;
            case 2:
                login_hunter();
                break;
            case 3:
                if (logged_in_index == -1) {
                    printf("Silakan login terlebih dahulu.\n");
                } else {
                    tampilkan_dungeon();
                }
                break;
            case 0:
                shmdt(shared_data);
                printf("Keluar dari program.\n");
                break;
            default:
                printf("Pilihan tidak valid.\n");
                break;
        }
    } while (pilihan != 0);
}

int main() {
    connect_shared_memory();
    menu();
    return 0;
}
```
pada hunter.c ini kita akan bisa registrasi sesuai dengan nama yang kita masukan, setelah itu kita harus login untuk bisa mengecek dungeon mana yang bisa kita masuki sesuai dengan shared memory dari system.c

hunter.c memiliki output sebagai berikut : 

![Image](https://github.com/user-attachments/assets/70520a9b-d5a6-42d2-98ee-b384aa2c15c6)

kesimpulan dari system.c dan hunter.c

system.c memiliki shared memory ke hunter.c sebagai data utama yang saling terhubung, sebagai hunter kelas 1 kita hanya bisa memasuki dungeon level 1 sesuai dengan system.c

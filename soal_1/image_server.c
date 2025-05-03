#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#define PORT 9090
#define BUF_SIZE 8192

char base_dir[PATH_MAX];

void build_path(char *dest, const char *subpath) {
    snprintf(dest, PATH_MAX, "%s/%s", base_dir, subpath);
}

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

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
    }
}

int hex_to_bytes(const char *hex, unsigned char *out) {
    int len = strlen(hex);
    for (int i = 0; i < len / 2; i++) {
        sscanf(hex + 2 * i, "%2hhx", &out[i]);
    }
    return len / 2;
}

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

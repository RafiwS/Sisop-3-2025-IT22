#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 9090
#define BUF_SIZE 8192

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

int main() {
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

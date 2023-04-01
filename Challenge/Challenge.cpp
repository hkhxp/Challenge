#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
//#define _CRT_SECURE_NO_WARNINGS 
#define MAX_BUFFER_SIZE 1024
#define INI_FILE "fconfig.ini" 
#define BUF_SIZE 2048
#define PRINT_INTERVAL 120 // in seconds

void handle_message(const char* message, sqlite3* db, char* p1, char* p2) {
    char P1message[256], P2message[256];

    if (strncmp(message, p1, strlen(p1)) == 0) {
        strncpy(P1message, message + strlen(p1), strlen(message) - strlen(p1));
        P1message[strlen(message) - strlen(p1)] = '\0';
        int num_value = atoi(P1message);
        char* sql = sqlite3_mprintf("INSERT INTO messages (value_int, time) VALUES ( %d, CURRENT_TIMESTAMP);", num_value);
        sqlite3_exec(db, sql, NULL, NULL, NULL);
        sqlite3_free(sql);
    }
    else if (strncmp(message, p2, strlen(p2)) == 0) {
        strncpy(P2message, message + strlen(p2), strlen(message) - strlen(p2));
        P2message[strlen(message) - strlen(p2)] = '\0';
        char* sql = sqlite3_mprintf("INSERT INTO messages (value_str, time) VALUES ('%q', CURRENT_TIMESTAMP);", P2message);
        sqlite3_exec(db, sql, NULL, NULL, NULL);
        sqlite3_free(sql);
    }
}


int main()
{
    sqlite3* db;
    char* error_message = NULL;

    FILE* fp = fopen(INI_FILE, "r");
    if (fp == NULL) {
        perror("Failed to open fconfig.ini");
        return EXIT_FAILURE;
    }
    char line[256];
    char p1[256];
    char p2[256];
    int port;
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    sscanf(line, "port = %d", &port);
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    sscanf(line, "p1 = %s", p1);
    fgets(line, sizeof(line), fp);
    sscanf(line, "p2 = %s", p2);
    fclose(fp);

    // Create SQLite database
    if (sqlite3_open("messagedb.db", &db) != SQLITE_OK) {
        printf("Failed to open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char* sql = "DROP TABLE IF EXISTS messages";

    int rc = sqlite3_exec(db, sql, 0, 0, &error_message);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", error_message);
        sqlite3_free(error_message);
    }
    // else {
    //     printf("Table removed successfully\n");
     //}

     // Create table
    // if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, value_int INTEGER, value_str TEXT, time TIMESTAMP);",
    if (sqlite3_exec(db, "CREATE TABLE messages (id INTEGER PRIMARY KEY AUTOINCREMENT, value_int INTEGER, value_str TEXT, time TIMESTAMP);", NULL, NULL, &error_message) != SQLITE_OK) {
        printf("Failed to create table: %s\n", error_message);
        sqlite3_free(error_message);
        sqlite3_close(db);
        return 1;
    }

    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char buffer[BUF_SIZE];
    int recvLen;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("Error initializing Winsock");
        exit(EXIT_FAILURE);
    }

    // Create UDP socket
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        perror("Error creating socket");
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    // Bind socket to server address and port
    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("Error binding socket");
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("The server is ready to receive on port %d ...\n", port);

    time_t lastPrintTime = time(NULL);

    while (1) {
        // Receive message from client
        if ((recvLen = recvfrom(serverSocket, buffer, BUF_SIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLen)) == SOCKET_ERROR) {
            perror("Error receiving message");
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        buffer[recvLen] = '\0';
        //printf("Received message: %s\n", buffer);

        handle_message(buffer, db, p1, p2);

        // Send message back to client
        if (sendto(serverSocket, buffer, recvLen, 0, (struct sockaddr*)&clientAddr, clientAddrLen) == SOCKET_ERROR) {
            perror("Error sending message");
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        //Phase 2
        time_t currentTime = time(NULL);

        if (currentTime - lastPrintTime >= PRINT_INTERVAL) {

            // Execute the SQL statement and retrieve the result set
            const char* sql = "SELECT * FROM messages where value_int IS NOT NULL ORDER BY value_int ASC";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                printf("Error preparing statement: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                return 1;
            }

            // Print the column headers
            printf("%-10s %-10s %-40s\n", "ID", "P1 value", "Time");

            // print each row
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* timestamp = (const char*)sqlite3_column_text(stmt, 0);
                const char* intvalue = (const char*)sqlite3_column_text(stmt, 1);
                const char* time_received = (const char*)sqlite3_column_text(stmt, 3);
                printf("%-10s %-10s %-40s\n", timestamp, intvalue, time_received);
            }

            sqlite3_finalize(stmt);

            printf("\n");

            lastPrintTime = currentTime;
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
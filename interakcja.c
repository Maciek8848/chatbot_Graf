#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib") // Linkowanie z biblioteką ws2_32

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BUFFER_SIZE 4096

// Funkcja do wysyłania żądania HTTP i odbierania odpowiedzi
char* send_http_request(const char *user_input) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char request[BUFFER_SIZE];
    char *response = NULL;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Inicjalizacja Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return NULL;
    }

    // Tworzenie gniazda
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    // Łączenie z serwerem
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    // Przygotowanie treści JSON
    char json_body[BUFFER_SIZE];
    snprintf(json_body, sizeof(json_body),
             "{\"model\": \"qwen2.5-7b-instruct-1m\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}], \"temperature\": 0.7, \"max_tokens\": 100}",
             user_input);

    // Przygotowanie żądania HTTP
    snprintf(request, sizeof(request),
             "POST /v1/chat/completions HTTP/1.1\r\n"
             "Host: 127.0.0.1:1234\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             strlen(json_body), json_body);

    // Wysyłanie żądania
    if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    // Odbieranie odpowiedzi
    response = (char *)malloc(BUFFER_SIZE);
    if (!response) {
        printf("Memory allocation failed!\n");
        closesocket(sock);
        WSACleanup();
        return NULL;
    }
    response[0] = '\0';

    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        response = (char *)realloc(response, strlen(response) + bytes_received + 1);
        strcat(response, buffer);
    }

    if (bytes_received == SOCKET_ERROR) {
        printf("Receive failed: %d\n", WSAGetLastError());
        free(response);
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    // Zamykanie połączenia
    closesocket(sock);
    WSACleanup();

    return response;
}

// Funkcja do sprawdzania, czy odpowiedź jest związana z grafem
int is_graph_related(const char *response) {
    return strstr(response, "graf") != NULL || strstr(response, "wierzchołek") != NULL || strstr(response, "krawędź") != NULL;
}

int main() {
    char user_input[BUFFER_SIZE];
    char modified_input[BUFFER_SIZE + 50]; // Dodajemy miejsce na "Wygeneruj graf"

    while (1) {
        printf("Wprowadź zapytanie (lub wpisz 'exit', aby zakończyć): ");
        fgets(user_input, BUFFER_SIZE, stdin);
        user_input[strcspn(user_input, "\n")] = 0; // Usuwamy znak nowej linii

        // Sprawdź, czy użytkownik chce wyjść
        if (strcmp(user_input, "exit") == 0) {
            printf("Zamykanie programu...\n");
            break;
        }

        // Dodajemy "Wygeneruj graf" do zapytania
        snprintf(modified_input, sizeof(modified_input), "Wygeneruj graf %s", user_input);

        // Wysyłanie zapytania do chatbota
        char *response = send_http_request(modified_input);

        if (response) {
            // Sprawdzanie, czy odpowiedź jest związana z grafem
            if (is_graph_related(response)) {
                printf("Chatbot wygenerował graf:\n%s\n", response);
            } else {
                printf("Odpowiedź nie jest związana z grafem. Spróbuj ponownie.\n");
            }

            free(response);
        } else {
            printf("Błąd podczas komunikacji z serwerem.\n");
        }
    }

    return 0;
}
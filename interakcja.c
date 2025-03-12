#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")  

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BUFFER_SIZE 32768

char* send_http_request(const char *user_input) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char request[BUFFER_SIZE];
    char *response = NULL;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return NULL;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);


    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    char json_body[BUFFER_SIZE];
    snprintf(json_body, sizeof(json_body),
             "{\"model\": \"qwen2.5-7b-instruct-1m\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}], \"temperature\": 0.1, \"max_tokens\": 400}",
             user_input);

    snprintf(request, sizeof(request),
             "POST /v1/chat/completions HTTP/1.1\r\n"
             "Host: 127.0.0.1:1234\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             strlen(json_body), json_body);

    if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

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

    closesocket(sock);
    WSACleanup();

    return response;
}


int is_graph_related(const char* user_input) {
    if (strstr(user_input, "graf") ||
        strstr(user_input, "graph") ||
        strstr(user_input, "generuj") ||
        strstr(user_input, "generowanie")) {
        return 1;
    }
    return 0;
}
// int is_graph_related(const char* user_input) {
//     char query[BUFFER_SIZE];
//     snprintf(query, BUFFER_SIZE,
//              "Czy następujące zapytanie ma coś wspólnego z grafem lub generowaniem grafu? Odpowiedz tylko 'tak' lub 'nie': %s",
//              user_input);

//     char *bot_response = send_http_request(query);
//     if (!bot_response) {
//         fprintf(stderr, "Błąd podczas komunikacji z serwerem.\n");
//         return 0;
//     }

//     printf("Odpowiedź chatbota: %s\n", bot_response);
//     int is_related = (strstr(bot_response, "tak") != NULL);
//     free(bot_response);

//     return is_related;
// }
char *replace_text_newline_with_enter(const char *str) {

    size_t length = 0;
    const char *ptr = str;
    while (*ptr) {
        if (strncmp(ptr, "\\n", 2) == 0) {
            length += 1; 
            ptr += 2;
        } else {
            length += 1;
            ptr += 1;
        }
    }


    char *result = (char *)malloc(length + 1); 
    if (result == NULL) {
        return NULL; 
    }


    char *dest = result;
    ptr = str;
    while (*ptr) {
        if (strncmp(ptr, "\\n", 2) == 0) {
            *dest++ = '\n'; 
            ptr += 2;
        } else {
            *dest++ = *ptr++; 
        }
    }
    *dest = '\0';

    return result;
}
char *extract_content(const char *input) {

    const char *content_start = strstr(input, "\"content\":");
    if (content_start == NULL) {
        return NULL;
    }

    content_start += strlen("\"content\":");


    const char *quote_start = strchr(content_start, '"');
    if (quote_start == NULL) {
        return NULL;
    }

    quote_start++;

    const char *quote_end = strchr(quote_start, '"');
    if (quote_end == NULL) {
        return NULL;
    }

    size_t length = quote_end - quote_start;


    char *result = (char *)malloc(length + 1);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, quote_start, length);
    result[length] = '\0';

    return replace_text_newline_with_enter(result);
}
int main() {
    char user_input[BUFFER_SIZE];
    char *response;
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             " %s Krawędzie są kierunkowe dlatego zapisuje się je ze strzałką, przykładowa odpowiedź: 0->1,3; 1->0; 2; 3->0,2 . Wypisz tylko połączenia",
             user_input);
    while (1) {
        printf("Aby wrócić do menu napisz (cofnij)\nWprowadź swoje zapytanie (lub wpisz 'exit' aby zakończyć): ");
        if (!fgets(query, BUFFER_SIZE, stdin)) {
            break;
        }
        query[strcspn(query, "\n")] = '\0'; 
        if (strcmp(query, "cofnij") == 0 || strcmp(query, "c")==0 ){
            system("program.exe");  
            break;
        }
        else if (strcmp(query, "exit") == 0 || strcmp(query, "quit") == 0) {
            break;
        }

        if (is_graph_related(query)) {
            response = send_http_request(query);
            if (!response) {
                fprintf(stderr, "Błąd podczas komunikacji z serwerem.\n");
                continue;
            }
            printf("Odpowiedź chatbota: %s\n", extract_content(response));
            free(response);
        } else {
            printf("Odpowiadam tylko na pytania związane z generowaniem grafu.\n");
        }
    }

    return 0;
}
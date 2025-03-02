#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_VERTICES 100
#define MAX_EDGES 500
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BUFFER_SIZE 8192

typedef struct { int src, dest; } Edge;
typedef struct { int vertices, edges; Edge edgeList[MAX_EDGES]; } Graph;

// Funkcje pomocnicze
void addEdge(Graph *graph, int src, int dest) {
    if (graph->edges < MAX_EDGES) {
        graph->edgeList[graph->edges++] = (Edge){src, dest};
    } else {
        printf("Maksymalna liczba krawędzi!\n");
    }
}

void printGraph(Graph *graph) {
    printf("Graf z %d wierzchołkami i %d krawędziami:\n", graph->vertices, graph->edges);
    for (int i = 0; i < graph->edges; i++) {
        printf("%d -> %d\n", graph->edgeList[i].src, graph->edgeList[i].dest);
    }
}

// Parsowanie odpowiedzi JSON
void extractContent(const char *json, char *output) {
    const char *pattern = "\"content\": \"";
    char *start = strstr(json, pattern);
    
    if (!start) {
        strcpy(output, "Błąd: Brak pola 'content'");
        return;
    }
    
    start += strlen(pattern);
    char *end = strchr(start, '"');
    
    if (!end) {
        strcpy(output, "Błąd: Niekompletne pole 'content'");
        return;
    }

    strncpy(output, start, end - start);
    output[end - start] = '\0';
}

// Parsowanie danych grafu
int parseGraphData(const char *text, int *v, int *e, int edges[][2]) {
    // Szukaj wzorca "X wierzchołkami i Y krawędziami"
    if (sscanf(text, "Graf z %d wierzchołkami i %d krawędziami:", v, e) != 2) {
        return 0;
    }

    // Znajdź listę krawędzi
    char *ptr = strchr(text, ':');
    if (!ptr) return 0;
    ptr++;

    int count = 0;
    while (*ptr && count < *e) {
        if (sscanf(ptr, "%d-%d", &edges[count][0], &edges[count][1]) == 2) {  // POPRAWIONE
            count++;
        }
        ptr = strpbrk(ptr, ", "); // Szukaj separatorów
        if (!ptr) break;
        ptr++;
    }

    return (count == *e);
}

// Komunikacja z API
char* sendRequest(const char *prompt) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    
    // Inicjalizacja Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Błąd inicjalizacji Winsock!\n");
        return NULL;
    }

    // Przygotuj żądanie HTTP
    char request[BUFFER_SIZE], json[512];
    snprintf(json, sizeof(json), 
        "{\"model\":\"llama-3.2-1b-instruct\","
        "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
        "\"temperature\":0.7}", prompt);

    // Utwórz gniazdo
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Błąd tworzenia gniazda!\n");
        WSACleanup();
        return NULL;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Połącz z serwerem
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Błąd połączenia!\n");
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    // Wyślij żądanie
    snprintf(request, sizeof(request),
        "POST /v1/chat/completions HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n%s",
        SERVER_IP, SERVER_PORT, strlen(json), json);

    send(sock, request, strlen(request), 0);

    // Odbiór odpowiedzi
    static char response[BUFFER_SIZE];
    int bytes = recv(sock, response, BUFFER_SIZE-1, 0);
    response[bytes] = '\0';

    closesocket(sock);
    WSACleanup();
    return response;
}

int main() {
    char input[256], content[BUFFER_SIZE];
    printf("Podaj zapytanie: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; // Usuń enter

    char *response = sendRequest(input);
    if (!response) {
        printf("Błąd komunikacji z API!\n");
        return 1;
    }

    extractContent(response, content);
    printf("Odpowiedź modelu:\n%s\n", content);

    // Parsuj i wyświetl graf
    int v, e, edges[MAX_EDGES][2];
    if (parseGraphData(content, &v, &e, edges)) {
        Graph g = {v, 0};
        for (int i = 0; i < e; i++) addEdge(&g, edges[i][0], edges[i][1]);
        printGraph(&g);
    } else {
        printf("Błąd parsowania odpowiedzi!\n");
    }

    return 0;
}
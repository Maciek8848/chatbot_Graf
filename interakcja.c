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

void addEdge(Graph *graph, int src, int dest) {
    if (graph->edges < MAX_EDGES) graph->edgeList[graph->edges++] = (Edge){src, dest};
    else printf("Maksymalna liczba krawędzi!\n");
}

void printGraph(Graph *graph) {
    printf("Graf z %d wierzchołkami i %d krawędziami:\n", graph->vertices, graph->edges);
    for (int i = 0; i < graph->edges; i++) printf("%d -> %d\n", graph->edgeList[i].src, graph->edgeList[i].dest);
}

void extractContent(const char *json, char *output) {
    const char *pattern = "\"content\": \"";
    char *start = strstr(json, pattern);
    if (!start) { strcpy(output, "Błąd: Brak pola 'content'"); return; }
    start += strlen(pattern);
    char *end = strchr(start, '"');
    if (!end) { strcpy(output, "Błąd: Niekompletne pole 'content'"); return; }
    strncpy(output, start, end - start);
    output[end - start] = '\0';
}

int parseGraphData(const char *text, int *v, int *e, int edges[][2]) {
    char *textCopy = _strdup(text);
    if (!textCopy) return 0;

    char *context = NULL;
    char *segment = strtok_s(textCopy, ";\n", &context);
    int count = 0;
    int max_vertex = 0;

    while (segment && count < MAX_EDGES) {
        // Usuń białe znaki wokół segmentu
        while (*segment == ' ') segment++;
        size_t len = strlen(segment);
        while (len > 0 && segment[len-1] == ' ') segment[--len] = '\0';

        // Podziel na źródło i cele
        char *dash = strchr(segment, '-');
        if (dash) {
            *dash = '\0';
            int src = atoi(segment);
            if (src > max_vertex) max_vertex = src;

            // Przetwarzaj kolejne cele
            char *destContext = NULL;
            char *destStr = strtok_s(dash + 1, ",", &destContext);
            while (destStr && count < MAX_EDGES) {
                int dest = atoi(destStr);
                edges[count][0] = src;
                edges[count][1] = dest;
                if (dest > max_vertex) max_vertex = dest;
                count++;
                destStr = strtok_s(NULL, ",", &destContext);
            }
        } else {
            // Tylko wierzchołek bez krawędzi
            int src = atoi(segment);
            if (src > max_vertex) max_vertex = src;
        }

        segment = strtok_s(NULL, ";\n", &context);
    }

    *v = max_vertex;
    *e = count;
    free(textCopy);
    return (count > 0);
}

char* sendRequest(const char *prompt) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { printf("Błąd Winsock!\n"); return NULL; }
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) { printf("Błąd gniazda!\n"); WSACleanup(); return NULL; }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) { printf("Błąd połączenia!\n"); closesocket(sock); WSACleanup(); return NULL; }

    char request[BUFFER_SIZE], json[512];
    snprintf(json, sizeof(json), "{\"model\":\"llama-3.2-1b-instruct\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"temperature\":0.7}", prompt);
    snprintf(request, sizeof(request), "POST /v1/chat/completions HTTP/1.1\r\nHost: %s:%d\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s", SERVER_IP, SERVER_PORT, strlen(json), json);
    send(sock, request, strlen(request), 0);

    static char response[BUFFER_SIZE];
    int bytes = recv(sock, response, BUFFER_SIZE-1, 0);
    response[bytes] = '\0';
    closesocket(sock);
    WSACleanup();
    return response;
}

int main() {
    char input[256] = "Generuj graf w postaci tekstowej, krawędzie bierz losowo chyba że w poleceniu będnie powiedziane inaczej.od jednego wieszchołka może iść kilka krawędzi. Przykład odpowidzi: 1-2,4 ; 2-1; 3;4-1,3(dla 4 wieszchołków i 5 połączeń)";
    char content[BUFFER_SIZE];
    // char input[256], content[BUFFER_SIZE];
    // printf("Podaj zapytanie: ");
    // fgets(input, sizeof(input), stdin);


    char *response = sendRequest(input);
    if (!response) { printf("Błąd API!\n"); return 1; }

    extractContent(response, content);
    printf("Odpowiedź modelu:\n%s\n", content);

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
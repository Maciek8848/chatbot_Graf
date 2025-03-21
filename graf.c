#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_VERTICES 500
#define MAX_EDGES 100

typedef struct {
    int src, dest;
} Edge;

typedef struct {
    int vertices;
    int edges;
    Edge edgeList[MAX_EDGES];
} Graph;

void addEdge(Graph *graph, int src, int dest) {
    // czy nowo dodana krawędź już istnieje?
    for (int i = 0; i < graph->edges; i++) {
        if (graph->edgeList[i].src == src && graph->edgeList[i].dest == dest) {
            return; // jak tak, to nie wykonuj dalej funkcji
        }
    }
    if (graph->edges < MAX_EDGES) {
        graph->edgeList[graph->edges].src = src;
        graph->edgeList[graph->edges].dest = dest;
        graph->edges++;
    } else {
        printf("Maksymalna liczba krawedzi została osiagnieta.\n");
        return;
    }
}

int compareEdges(const void *a, const void *b) {
    Edge *edgeA = (Edge *)a;
    Edge *edgeB = (Edge *)b;
    if (edgeA->src == edgeB->src) {
        return edgeA->dest - edgeB->dest;
    }
    return edgeA->src - edgeB->src;
}

void printGraph(Graph *graph) {
    printf("Graf z %d wierzcholkami i %d krawedziami:\n", graph->vertices, graph->edges/2);

    qsort(graph->edgeList, graph->edges, sizeof(Edge), compareEdges);

    int currentSrc = -1;
    for (int i = 0; i < graph->edges; i++) {
        if (graph->edgeList[i].src != currentSrc) {
            if (currentSrc != -1) {
                printf("\n");
            }
            currentSrc = graph->edgeList[i].src;
            printf("%d -> %d", graph->edgeList[i].src, graph->edgeList[i].dest);
        } else {
            printf(", %d", graph->edgeList[i].dest);
        }
    }
    printf("\n");
}

void generateRandomGraph(Graph *graph) {
    srand(time(NULL));
    graph->edges = 0;
    for (int i = 0; i < graph->vertices; i++) {
        if (graph->edges >= MAX_EDGES) {
            break;
        }
        int edges = rand() % (graph->vertices - 1) + 1;
        for (int j = 0; j < edges; j++) {
            int dest = rand() % graph->vertices;
            if (graph->edges >= MAX_EDGES) {
                printf("Maksymalna liczba krawedzi została osiagnieta. Dalsze krawedzie nie zostana dodane.\n");
                break;
            }
            if (dest != i) {
                addEdge(graph, i, dest);
                //tutaj dodałem, żeby było dwukierunkowe
                addEdge(graph, dest, i);
            }
        }
    }
}

int isValidNumber(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    return 1;
}

void saveGraph(Graph *graph, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file");
        return;
    }
    
    fprintf(fp, "Graf z %d wierzcholkami i %d krawedziami:\n", graph->vertices, graph->edges / 2);
    
    qsort(graph->edgeList, graph->edges, sizeof(Edge), compareEdges);
    
    int currentSrc = -1;
    for (int i = 0; i < graph->edges; i++) {
        if (graph->edgeList[i].src != currentSrc) {
            if (currentSrc != -1) {
                fprintf(fp, "\n");
            }
            currentSrc = graph->edgeList[i].src;
            fprintf(fp, "%d -> %d", graph->edgeList[i].src, graph->edgeList[i].dest);
        } else {
            fprintf(fp, ", %d", graph->edgeList[i].dest);
        }
    }
    fprintf(fp, "\n");
    
    fclose(fp);
}

void interactiveMode() {
    Graph graph;
    graph.edges = 0;
    char input[50];
    char choice[10];

    printf("Podaj liczbe wierzcholkow: ");
    scanf("%s", input);
    if (!isValidNumber(input)) {
        printf("[!] ERROR: niepoprawna liczba wierzcholkow.\n");
        return;
    }
    graph.vertices = atoi(input);

    printf("Czy graf ma byc losowy? (tak/nie): ");
    scanf("%s", input);
    if (strcmp(input, "tak") == 0 || strcmp(input, "t") == 0) {
        generateRandomGraph(&graph);
    } else {
        printf("Podaj liczbe krawedzi: ");
        scanf("%s", input);
        if (!isValidNumber(input)) {
            printf("[!] ERROR: niepoprawna liczba krawedzi.\n");
            return;
        }
        int numEdges = atoi(input);  //oddzielna zmienna, do liczby krawedzi

        for (int i = 0; i < numEdges; i++) {
            if (graph.edges >= MAX_EDGES) {
                printf("Maksymalna liczba krawedzi została osiagnieta. Dalsze krawedzie nie zostana dodane.\n");
                break;
            }
            int src, dest;
            printf("Podaj krawedz %d (zrodlo i cel): ", i + 1);

            if (scanf("%d %d", &src, &dest) != 2) {
                printf("[!] ERROR: niepoprawna wartosc zrodla lub celu.\n");
                return;
            }

            if (src >= graph.vertices || dest >= graph.vertices || src < 0 || dest < 0) {
                printf("[!] ERROR: niepoprawne wartosci krawedzi.\n");
                return;
            }
            //tutaj dodałem, żeby było dwukierunkowe
            addEdge(&graph, src, dest);
            addEdge(&graph, dest, src);
        }
    }
    printGraph(&graph);
    printf("Czy zapisac graf do pliku? (tak/nie): ");
    scanf("%s", choice);
    if (strcmp(choice, "tak") == 0 || strcmp(choice, "t") == 0) {
    saveGraph(&graph, "graph_output.txt");
    printf("Graf zapisany do 'graph_output.txt'\n");
}

}

int main(int argc, char *argv[]) {
    if (argc > 1) { // podal argumenty
        if (argc > 3 || !isValidNumber(argv[1])) {
            printf("Uzycie: %s <liczba_wierzcholkow> [plik_wyjsciowy]\n", argv[0]);
            return 1;
        }
        Graph graph;
        graph.vertices = atoi(argv[1]);
        graph.edges = 0;
        generateRandomGraph(&graph);
        printGraph(&graph);

        if (argc == 3) {
            saveGraph(&graph, argv[2]);
            printf("Graf zapisany do '%s'\n", argv[2]);
        }

    } else {
        interactiveMode();
    }
    return 0;
}
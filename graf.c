#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_VERTICES 100
#define MAX_EDGES 500

typedef struct {
    int src, dest;
} Edge;

typedef struct {
    int vertices;
    int edges;
    Edge edgeList[MAX_EDGES];
} Graph;

void addEdge(Graph *graph, int src, int dest) {
    if (graph->edges < MAX_EDGES) {
        graph->edgeList[graph->edges].src = src;
        graph->edgeList[graph->edges].dest = dest;
        graph->edges++;
    } else {
        printf("Maksymalna liczba krawedzi została osiagnieta.\n");
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
    printf("Graf z %d wierzcholkami i %d krawedziami:\n", graph->vertices, graph->edges);
    
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
            printf(" %d", graph->edgeList[i].dest);
        }
    }
    printf("\n");
}

int isValidNumber(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    return 1;
}

void generateRandomGraph(Graph *graph) {
    srand(time(NULL));
    graph->edges = 0;
    for (int i = 0; i < graph->vertices; i++) {
        int edges = rand() % (graph->vertices - 1) + 1;
        for (int j = 0; j < edges; j++) {
            int dest = rand() % graph->vertices;
            if (dest != i) {
                addEdge(graph, i, dest);
            }
        }
    }
}


void interactiveMode() {

    Graph graph;
    graph.edges = 0;
    char input[50];
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

        generateRandomGraph( & graph);

    } else {
        printf("Podaj liczbe krawedzi: ");
        scanf("%s", input);

        if (!isValidNumber(input)) {

            printf("[!] ERROR: niepoprawna liczba krawedzi.\n");
            return;
        }
        graph.edges = atoi(input);

        for (int i = 0; i < graph.edges; i++) {
            int src, dest;
            printf("Podaj krawedz %d (zrodlo i cel): ", i + 1);
            scanf("%d %d", & src, & dest);
            if (src >= graph.vertices || dest >= graph.vertices || src < 0 || dest < 0) {
                printf("[!] ERROR: niepoprawne wartosci krawedzi.\n");
                return;
            }
            addEdge( & graph, src, dest);
        }
    }
    printGraph( & graph);

}

int main(int argc, char *argv[]) {
    //if (argc > 3) {
    //    printf("Uzycie: %s <liczba_wierzcholkow> <liczba_krawedzi> [src1 dest1 src2 dest2 ...]\n", argv[0]);
    //    return 1;
    // }

    Graph graph;
    graph.vertices = atoi(argv[1]);
    graph.edges = 0;
    
        if (argc < 2) {
        interactiveMode();
        return 7;
    }

    int edges = atoi(argv[2]);
    if (edges <= 0 || edges > MAX_EDGES) {
        printf("Niepoprawna liczba krawedzi. Musi byc w zakresie 1-%d.\n", MAX_EDGES);
        return 1;
    }

    if (argc != 3 + 2 * edges) {
        printf("Niepoprawna liczba argumentow. Oczekiwano %d argumentow (src1 dest1 src2 dest2 ...).\n", 2 * edges);
        return 1;
    }

    for (int i = 0; i < edges; i++) {
        int src = atoi(argv[3 + 2 * i]);
        int dest = atoi(argv[3 + 2 * i + 1]);
        if (src < 0 || src >= graph.vertices || dest < 0 || dest >= graph.vertices) {
            printf("Niepoprawne wartosci krawedzi: %d -> %d.\n", src, dest);
            return 1;
        }
        addEdge(&graph, src, dest);
    }


 
    return 0;
}

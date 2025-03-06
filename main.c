#include <stdio.h>
#include <stdlib.h>

int main() {
    int wybor;

    printf("Wybierz program do uruchomienia:\n");
    printf("1. graf.c\n");
    printf("2. interakcja.c\n");
    printf("Wprowadź numer programu (1 lub 2): ");

    if (scanf("%d", &wybor) != 1) {
        printf("Błąd wczytywania danych.\n");
        return 1;
    }

    switch (wybor) {
        case 1:
            system("graf.exe");  
            break;
        case 2:

            system("interakcja.exe");  

            break;
        default:
            printf("Niepoprawny wybór.\n");
            break;
    }

    return 0;
}
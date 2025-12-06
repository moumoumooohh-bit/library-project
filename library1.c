/*********************************************************
 *  LIBRARY SERVER 1 - Version améliorée
 *  Recherche fonctionnelle sur mots ou titre complet
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 2048

typedef struct {
    int id;
    char title[50];
    char author[50];
    int leased;
    int search_count;
} Book;

Book books[] = {
    {101, "C Programming", "Dennis Ritchie", 0, 0},
    {102, "Operating Systems", "Andrew Tanenbaum", 0, 0},
    {103, "Linux System Design", "Robert Love", 0, 0},
    {104, "Distributed Systems", "George Coulouris", 0, 0}
};

int book_count = 4;
int total_searches = 0;

// Convertir chaîne en minuscules
void str_tolower(char *s) {
    for (int i = 0; s[i]; i++)
        s[i] = tolower(s[i]);
}

// Rechercher un mot-clé (par mot ou titre complet)
void handle_search(char *keyword, char *response) {
    strcpy(response, "");
    total_searches++;

    while (*keyword == ' ') keyword++; // enlever espaces début
    keyword[strcspn(keyword, "\n")] = '\0'; // enlever saut ligne

    char key_lower[100];
    strcpy(key_lower, keyword);
    str_tolower(key_lower);

    for (int i = 0; i < book_count; i++) {
        char title_lower[100], author_lower[100];
        strcpy(title_lower, books[i].title);
        strcpy(author_lower, books[i].author);
        str_tolower(title_lower);
        str_tolower(author_lower);

        int found = 0;

        // Recherche par mot (séparer keyword en mots)
        char key_copy[100];
        strcpy(key_copy, key_lower);
        char *token = strtok(key_copy, " ");
        while (token != NULL) {
            if (strstr(title_lower, token) || strstr(author_lower, token)) {
                found = 1;
                break;
            }
            token = strtok(NULL, " ");
        }

        if (found) {
            books[i].search_count++;
            char line[200];
            sprintf(line,
                "ID: %d | %s | %s | %s\n",
                books[i].id,
                books[i].title,
                books[i].author,
                books[i].leased ? "LOUE" : "DISPONIBLE"
            );
            strcat(response, line);
        }
    }
}

// Louer un livre
void handle_lease(int id, char *response) {
    for (int i = 0; i < book_count; i++) {
        if (books[i].id == id) {
            if (books[i].leased)
                sprintf(response, "Livre %d déjà loué.\n", id);
            else {
                books[i].leased = 1;
                sprintf(response, "Livre %d loué avec succès.\n", id);
            }
            return;
        }
    }
    strcpy(response, "NOT_FOUND");
}

// Retourner un livre
void handle_return(int id, char *response) {
    for (int i = 0; i < book_count; i++) {
        if (books[i].id == id) {
            if (!books[i].leased)
                sprintf(response, "Livre %d n'est pas loué.\n", id);
            else {
                books[i].leased = 0;
                sprintf(response, "Livre %d retourné avec succès.\n", id);
            }
            return;
        }
    }
    strcpy(response, "NOT_FOUND");
}

// Lister tous les livres
void handle_list(char *response) {
    strcpy(response, "");
    for (int i = 0; i < book_count; i++) {
        char line[200];
        sprintf(line,
            "ID: %d | %s | %s | %s\n",
            books[i].id,
            books[i].title,
            books[i].author,
            books[i].leased ? "LOUE" : "DISPONIBLE"
        );
        strcat(response, line);
    }
}

// Statistiques locales
void handle_stats(char *response) {
    int max_index = 0;
    for (int i = 1; i < book_count; i++) {
        if (books[i].search_count > books[max_index].search_count)
            max_index = i;
    }

    sprintf(response,
        "\n=== STATISTIQUES LIBRARY SERVER 1 ===\n"
        "Recherches totales : %d\n"
        "Livre le plus recherché : %s (%d recherches)\n\n",
        total_searches,
        books[max_index].title,books[max_index].search_count
    );
}

// MAIN
int main() {
    int sockfd, clientfd;
    struct sockaddr_in serv, client;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(6001);
    serv.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&serv, sizeof(serv));
    listen(sockfd, 5);

    printf("Library Server 1 running on port 6001...\n");

    socklen_t client_len = sizeof(client);

    while (1) {
        clientfd = accept(sockfd, (struct sockaddr *)&client, &client_len);

        int len = recv(clientfd, buffer, BUFFER_SIZE, 0);
        buffer[len] = '\0';
        printf("[Coordinator] %s\n", buffer);

        if (strncmp(buffer, "search", 6) == 0)
            handle_search(buffer + 7, response);
        else if (strncmp(buffer, "lease", 5) == 0)
            handle_lease(atoi(buffer + 6), response);
        else if (strncmp(buffer, "return", 6) == 0)
            handle_return(atoi(buffer + 7), response);
        else if (strncmp(buffer, "list", 4) == 0)
            handle_list(response);
        else if (strncmp(buffer, "stats", 5) == 0)
            handle_stats(response);

        send(clientfd, response, strlen(response), 0);
        close(clientfd);
    }

    close(sockfd);
    return 0;
}

/*********************************************************
 *  LIBRARY SERVER 3 - Version améliorée
 *  Recherche par mot ou titre/auteur complet
 *********************************************************/

#include <stdio.h>      // Pour printf(), sprintf()
#include <stdlib.h>     // Pour atoi(), exit()
#include <string.h>     // Pour strcpy(), strcat(), strstr()
#include <unistd.h>     // Pour close(), read(), write()
#include <arpa/inet.h>  // Pour socket(), bind(), accept()
#include <ctype.h>      // Pour tolower()

#define BUFFER_SIZE 2048   // Taille du buffer d’échange

// ---------- STRUCTURE D’UN LIVRE ----------
typedef struct {
    int id;                 // Identifiant unique du livre
    char title[50];         // Titre du livre
    char author[50];        // Auteur du livre
    int leased;             // 1 = loué, 0 = disponible
    int search_count;       // Nombre de recherches sur ce livre
} Book;

// ---------- BASE DE DONNÉES DES LIVRES ----------
Book books[] = {
    {301, "Distributed Systems", "Tanenbaum", 0, 0},
    {302, "Computer Networks", "Andrew Tanenbaum", 0, 0},
    {303, "Cybersecurity Basics", "William Stallings", 0, 0},
    {304, "Cloud Computing", "Rajkumar Buyya", 0, 0}
};

int book_count = 4;         // Nombre total de livres
int total_searches = 0;     // Nombre total de recherches effectuées

// ---------- Convertir une chaîne en minuscules ----------
void str_tolower(char *s) {
    for (int i = 0; s[i]; i++)   // Parcours chaque caractère
        s[i] = tolower(s[i]);    // Converti en minuscule
}

// ---------- GÉRER RECHERCHE ----------
void handle_search(char *keyword, char *response) {

    strcpy(response, "");   // Effacer réponse précédente
    total_searches++;       // Incrémenter total des recherches

    while (*keyword == ' ') keyword++;   // Enlever espaces au début
    keyword[strcspn(keyword, "\n")] = '\0'; // Retirer le \n

    char key_lower[100];
    strcpy(key_lower, keyword);  // Copier mot clé
    str_tolower(key_lower);      // Convertir en minuscule

    for (int i = 0; i < book_count; i++) {

        char title_lower[100];
        char author_lower[100];

        strcpy(title_lower, books[i].title);   // Copier titre
        strcpy(author_lower, books[i].author); // Copier auteur

        str_tolower(title_lower);   // Minuscule
        str_tolower(author_lower);  // Minuscule

        int found = 0;   // 1 = livre trouvé

        // Recherche exacte dans titre ou auteur
        if (strstr(title_lower, key_lower) || strstr(author_lower, key_lower)) {
            found = 1;
        } 
        else {
            // Recherche par mots
            char key_copy[100];
            strcpy(key_copy, key_lower);

            char *token = strtok(key_copy, " ");   // Séparer par mots
            while (token != NULL) {
                if (strstr(title_lower, token) || strstr(author_lower, token)) {
                    found = 1;
                    break;
                }
                token = strtok(NULL, " ");
            }
        }

        if (found) {
            books[i].search_count++;

            char line[200];
            sprintf(line, "ID: %d | %s | %s | %s\n",
                books[i].id,
                books[i].title,
                books[i].author,
                books[i].leased ? "LOUE" : "DISPONIBLE"
            );

            strcat(response, line);  // Ajouter à la réponse
        }
    }
}

// ---------- GÉRER LOCATION ----------
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

    strcpy(response, "NOT_FOUND");   // Livre inconnu
}

// ---------- GÉRER RETOUR ----------
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

// ---------- LISTER TOUS LES LIVRES ----------
void handle_list(char *response) {

    strcpy(response, "");

    for (int i = 0; i < book_count; i++) {

        char line[200];

        sprintf(line, "ID: %d | %s | %s | %s\n",
            books[i].id,
            books[i].title,
            books[i].author,
            books[i].leased ? "LOUE" : "DISPONIBLE"
        );

        strcat(response, line);
    }
}

// ---------- STATISTIQUES ----------
void handle_stats(char *response) {

    int max_index = 0;

    for (int i = 1; i < book_count; i++) {
        if (books[i].search_count > books[max_index].search_count)
            max_index = i;
    }

    sprintf(response,
        "\n=== STATISTIQUES LIBRARY SERVER 3 ===\n"
        "Recherches totales : %d\n"
        "Livre le plus recherché : %s (%d recherches)\n\n",
        total_searches,
        books[max_index].title,
        books[max_index].search_count
    );
}

// ---------- MAIN ----------
int main() {

    int sockfd, clientfd;                 // Sockets
    struct sockaddr_in serv, client;      // Adresses
    char buffer[BUFFER_SIZE];             // Buffer réception
    char response[BUFFER_SIZE];           // Buffer réponse

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Créer socket TCP

    serv.sin_family = AF_INET;            // IPv4
    serv.sin_port = htons(6003);          // Port serveur
    serv.sin_addr.s_addr = INADDR_ANY;    // Accepte toutes IP

    bind(sockfd, (struct sockaddr *)&serv, sizeof(serv));  // Binder

    listen(sockfd, 5);                    // Écouter 5 clients max

    printf("Library Server 3 running on port 6003...\n");

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

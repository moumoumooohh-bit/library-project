/*********************************************************
 *  LIBRARY SERVER 1 - Version améliorée
 *  Recherche fonctionnelle sur mots ou titre complet
 *********************************************************/

#include <stdio.h>      // Fonctions d'affichage et manipulation standard
#include <stdlib.h>     // Fonctions utilitaires (atoi, malloc, etc.)
#include <string.h>     // Pour strcpy, strcat, strstr, strtok
#include <unistd.h>     // Pour close, read, write
#include <arpa/inet.h>  // Pour socket, bind, accept, structures réseau
#include <ctype.h>      // Pour tolower()

#define BUFFER_SIZE 2048    // Taille du buffer pour les messages

// ---------- STRUCTURE D’UN LIVRE ----------
typedef struct {
    int id;                 // ID unique du livre
    char title[50];         // Titre
    char author[50];        // Auteur
    int leased;             // 1 = loué, 0 = disponible
    int search_count;       // Nombre de fois où ce livre a été recherché
} Book;

// ---------- LISTE DES LIVRES ----------
Book books[] = {
    {101, "C Programming", "Dennis Ritchie", 0, 0},
    {102, "Operating Systems", "Andrew Tanenbaum", 0, 0},
    {103, "Linux System Design", "Robert Love", 0, 0},
    {104, "Distributed Systems", "George Coulouris", 0, 0}
};

int book_count = 4;         // Nombre total de livres
int total_searches = 0;     // Nombre global de recherches

// ---------- Convertir une chaîne en minuscules ----------
void str_tolower(char *s) {
    for (int i = 0; s[i]; i++)      // Parcours chaque caractère
        s[i] = tolower(s[i]);       // Conversion en minuscule
}

// ---------- RECHERCHE PAR MOTS OU TITRE COMPLET ----------
void handle_search(char *keyword, char *response) {

    strcpy(response, "");           // Initialiser la réponse vide
    total_searches++;               // Incrémenter le compteur global

    while (*keyword == ' ') keyword++;     // Retirer espaces au début
    keyword[strcspn(keyword, "\n")] = '\0'; // Supprimer le \n final

    char key_lower[100];
    strcpy(key_lower, keyword);      // Copier la clé
    str_tolower(key_lower);          // La mettre en minuscule

    for (int i = 0; i < book_count; i++) {

        char title_lower[100];
        char author_lower[100];

        strcpy(title_lower, books[i].title);   // Copier titre
        strcpy(author_lower, books[i].author); // Copier auteur

        str_tolower(title_lower);              // Minuscule titre
        str_tolower(author_lower);             // Minuscule auteur

        int found = 0;                         // Livre trouvé ? 0 = non

        char key_copy[100];
        strcpy(key_copy, key_lower);           // Copie du mot-clé

        char *token = strtok(key_copy, " ");   // Découper par mots

        while (token != NULL) {

            if (strstr(title_lower, token)     // Si mot dans titre
             || strstr(author_lower, token)) { // Ou dans auteur

                found = 1;
                break;
            }

            token = strtok(NULL, " ");         // Mot suivant
        }

        if (found) {                            // Si livre trouvé

            books[i].search_count++;           // +1 recherche pour ce livre

            char line[200];
            sprintf(line,                       // Construire la ligne
                "ID: %d | %s | %s | %s\n",
                books[i].id,
                books[i].title,
                books[i].author,
                books[i].leased ? "LOUE" : "DISPONIBLE"
            );

            strcat(response, line);             // Ajouter à la réponse
        }
    }
}

// ---------- LOUER UN LIVRE ----------
void handle_lease(int id, char *response) {

    for (int i = 0; i < book_count; i++) {

        if (books[i].id == id) {               // Livre trouvé

            if (books[i].leased)               // Si déjà loué
                sprintf(response, "Livre %d déjà loué.\n", id);

            else {
                books[i].leased = 1;           // Louer le livre
                sprintf(response, "Livre %d loué avec succès.\n", id);
            }
            return;
        }
    }

    strcpy(response, "NOT_FOUND");             // Livre introuvable
}

// ---------- RETOURNER UN LIVRE ----------
void handle_return(int id, char *response) {

    for (int i = 0; i < book_count; i++) {

        if (books[i].id == id) {               // Livre trouvé

            if (!books[i].leased)              // Déjà disponible ?
                sprintf(response, "Livre %d n'est pas loué.\n", id);

            else {
                books[i].leased = 0;           // Rendre disponible
                sprintf(response, "Livre %d retourné avec succès.\n", id);
            }
            return;
        }
    }

    strcpy(response, "NOT_FOUND");
}

// ---------- LISTER LES LIVRES ----------
void handle_list(char *response) {

    strcpy(response, "");                      // Réponse vide

    for (int i = 0; i < book_count; i++) {

        char line[200];

        sprintf(line,                           // Construire ligne
            "ID: %d | %s | %s | %s\n",
            books[i].id,
            books[i].title,
            books[i].author,
            books[i].leased ? "LOUE" : "DISPONIBLE"
        );

        strcat(response, line);                 // Ajouter ligne
    }
}

// ---------- STATISTIQUES ----------
void handle_stats(char *response) {

    int max_index = 0;                         // Index du livre le plus recherché

    for (int i = 1; i < book_count; i++) {

        if (books[i].search_count > books[max_index].search_count)
            max_index = i;                     // Mettre à jour
    }

    sprintf(response,                          // Créer le message
        "\n=== STATISTIQUES LIBRARY SERVER 1 ===\n"
        "Recherches totales : %d\n"
        "Livre le plus recherché : %s (%d recherches)\n\n",
        total_searches,
        books[max_index].title,
        books[max_index].search_count
    );
}

// ---------- MAIN : SERVEUR ----------
int main() {

    int sockfd, clientfd;                    // Sockets serveur et client
    struct sockaddr_in serv, client;         // Structures d’adresses
    char buffer[BUFFER_SIZE];                // Buffer de réception
    char response[BUFFER_SIZE];              // Buffer de réponse

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Création socket TCP

    serv.sin_family = AF_INET;               // IPv4
    serv.sin_port = htons(6001);             // Port du serveur
    serv.sin_addr.s_addr = INADDR_ANY;       // Accepter toutes IP

    bind(sockfd, (struct sockaddr *)&serv, sizeof(serv));   // Associer socket

    listen(sockfd, 5);                       // Écouter 5 connexions max

    printf("Library Server 1 running on port 6001...\n");

    socklen_t client_len = sizeof(client);

    while (1) {                               // Boucle infinie serveur

        clientfd = accept(sockfd, (struct sockaddr *)&client, &client_len);

        int len = recv(clientfd, buffer, BUFFER_SIZE, 0);  // Lire requête

        buffer[len] = '\0';                   // Terminer chaîne
        printf("[Coordinator] %s\n", buffer); // Afficher requête

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

        send(clientfd, response, strlen(response), 0);   // Envoyer réponse

        close(clientfd);                                 // Fermer client
    }

    close(sockfd);                                       // Fermer serveur
    return 0;
}

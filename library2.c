/*********************************************************
 *  LIBRARY SERVER 2 - Version améliorée COMMENTÉ LIGNE PAR LIGNE
 *********************************************************/

#include <stdio.h>      // Inclusion pour printf(), sprintf(), etc.
#include <stdlib.h>     // Inclusion pour atoi(), exit(), etc.
#include <string.h>     // Inclusion pour strcpy(), strcat(), strstr(), etc.
#include <unistd.h>     // Inclusion pour close(), read(), write()
#include <arpa/inet.h>  // Inclusion pour les sockets TCP/IP
#include <ctype.h>      // Inclusion pour tolower()

#define BUFFER_SIZE 2048    // Taille max d'un message envoyé ou reçu

// Définition de la structure du livre
typedef struct {
    int id;                 // Identifiant du livre
    char title[50];         // Titre du livre
    char author[50];        // Auteur du livre
    int leased;             // 1 = loué, 0 = disponible
    int search_count;       // Nombre de recherches locales
} Book;

// Liste des livres du serveur 2
Book books[] = {
    {201, "Database Systems", "Elmasri & Navathe", 0, 0},
    {202, "Computer Networks", "Andrew Tanenbaum", 0, 0},
    {203, "Artificial Intelligence", "Peter Norvig", 0, 0},
    {204, "Machine Learning", "Tom Mitchell", 0, 0}
};

int book_count = 4;         // Nombre total de livres
int total_searches = 0;     // Nombre de recherches sur ce serveur

// Fonction : convertir une chaîne en minuscules
void str_tolower(char *s) {
    for (int i = 0; s[i]; i++)   // Parcourt chaque caractère
        s[i] = tolower(s[i]);    // Convertit en minuscule
}

// Fonction de recherche (titre complet, auteur complet ou mots)
void handle_search(char *keyword, char *response) {

    strcpy(response, "");        // Efface la réponse précédente
    total_searches++;            // Incrémente le compteur de recherches

    while (*keyword == ' ')      // Supprime les espaces au début
        keyword++;

    keyword[strcspn(keyword, "\n")] = '\0';   // Supprime le \n final

    char key_lower[100];         // Copie du mot clé en minuscule
    strcpy(key_lower, keyword);
    str_tolower(key_lower);

    // --- Boucle sur tous les livres ---
    for (int i = 0; i < book_count; i++) {

        char title_lower[100];   // Titre du livre en minuscule
        char author_lower[100];  // Auteur du livre en minuscule

        strcpy(title_lower, books[i].title);
        strcpy(author_lower, books[i].author);

        str_tolower(title_lower);   // Convertir en minuscule
        str_tolower(author_lower);

        int found = 0;           // Flag → livre trouvé ?

        // Recherche titre/auteur complet
        if (strstr(title_lower, key_lower) || strstr(author_lower, key_lower)) {
            found = 1;
        } else {
            // Recherche par mots
            char key_copy[100];
            strcpy(key_copy, key_lower);

            char *token = strtok(key_copy, " ");   // Découpe chaque mot

            while (token != NULL) {                // Boucle sur les mots
                if (strstr(title_lower, token) || strstr(author_lower, token)) {
                    found = 1;                     // Mot trouvé → match
                    break;
                }
                token = strtok(NULL, " ");
            }
        }

        // Si trouvé → ajouter à la réponse
        if (found) {
            books[i].search_count++;   // Incrémenter statistiques locales

            char line[200];
            sprintf(line,
                "ID: %d | %s | %s | %s\n",
                books[i].id,
                books[i].title,
                books[i].author,
                books[i].leased ? "LOUE" : "DISPONIBLE"
            );

            strcat(response, line);    // Ajouter à la réponse finale
        }
    }
}

// Fonction de location d’un livre
void handle_lease(int id, char *response) {

    for (int i = 0; i < book_count; i++) {   // Chercher le livre
        if (books[i].id == id) {

            if (books[i].leased)             // Si déjà loué
                sprintf(response, "Livre %d déjà loué.\n", id);
            else {
                books[i].leased = 1;         // Passer à loué
                sprintf(response, "Livre %d loué avec succès.\n", id);
            }
            return;
        }
    }

    strcpy(response, "NOT_FOUND");           // Livre inexistant
}

// Fonction de retour d’un livre
void handle_return(int id, char *response) {

    for (int i = 0; i < book_count; i++) {   // Chercher le livre
        if (books[i].id == id) {

            if (!books[i].leased)            // Si pas loué
                sprintf(response, "Livre %d n'est pas loué.\n", id);
            else {
                books[i].leased = 0;         // Passer à disponible
                sprintf(response, "Livre %d retourné avec succès.\n", id);
            }
            return;
        }
    }

    strcpy(response, "NOT_FOUND");           // Livre inexistant
}

// Fonction listant tous les livres
void handle_list(char *response) {

    strcpy(response, "");                    // Vider la réponse

    for (int i = 0; i < book_count; i++) {   // Parcourir les livres
        char line[200];

        sprintf(line,
            "ID: %d | %s | %s | %s\n",
            books[i].id,
            books[i].title,
            books[i].author,
            books[i].leased ? "LOUE" : "DISPONIBLE"
        );

        strcat(response, line);              // Ajouter à la réponse
    }
}

// Fonction de statistiques locales
void handle_stats(char *response) {

    int max_index = 0;                       // Index du livre le plus recherché

    for (int i = 1; i < book_count; i++) {   // Trouver le max
        if (books[i].search_count > books[max_index].search_count)
            max_index = i;
    }

    sprintf(response,
        "\n=== STATISTIQUES LIBRARY SERVER 2 ===\n"
        "Recherches totales : %d\n"
        "Livre le plus recherché : %s (%d recherches)\n\n",
        total_searches,
        books[max_index].title,
        books[max_index].search_count
    );
}

// --- MAIN : point d'entrée du serveur ---
int main() {

    int sockfd, clientfd;                    // Sockets
    struct sockaddr_in serv, client;         // Adresse du serveur et client
    char buffer[BUFFER_SIZE];                // Message reçu
    char response[BUFFER_SIZE];              // Message à envoyer

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Création du socket

    serv.sin_family = AF_INET;                // IPv4
    serv.sin_port = htons(6002);              // Port du serveur 2
    serv.sin_addr.s_addr = INADDR_ANY;        // Accepter toutes IP

    bind(sockfd, (struct sockaddr *)&serv, sizeof(serv));  // Attacher socket → port

    listen(sockfd, 5);                        // Mettre socket en écoute

    printf("Library Server 2 running on port 6002...\n");

    socklen_t client_len = sizeof(client);

    // --- Boucle infinie : accepter et traiter les requêtes ---
    while (1) {

        clientfd = accept(sockfd, (struct sockaddr *)&client, &client_len);

        int len = recv(clientfd, buffer, BUFFER_SIZE, 0);   // Lire requête
        buffer[len] = '\0';                                 // Terminer chaîne

        printf("[Coordinator] %s\n", buffer);               // Afficher requête

        if (strncmp(buffer, "search", 6) == 0)
            handle_search(buffer + 7, response);            // Appel recherche
        else if (strncmp(buffer, "lease", 5) == 0)
            handle_lease(atoi(buffer + 6), response);       // Appel location
        else if (strncmp(buffer, "return", 6) == 0)
            handle_return(atoi(buffer + 7), response);      // Appel retour
        else if (strncmp(buffer, "list", 4) == 0)
            handle_list(response);                          // Appel liste
        else if (strncmp(buffer, "stats", 5) == 0)
            handle_stats(response);                         // Appel stats

        send(clientfd, response, strlen(response), 0);       // Envoyer réponse
        close(clientfd);                                     // Fermer client
    }

    close(sockfd);                                            // Fermer serveur
    return 0;
}

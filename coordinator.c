/*********************************************************
 *  COORDINATOR SERVER
 *  ------------------
 *  - Reçoit les requêtes du client
 *  - Diffuse les recherches à tous les Library Servers
 *  - Agrège les résultats
 *  - Transmet les demandes de lease/return
 *  - Ignore les serveurs non-répondants
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_SERVERS 3        // Nombre de Library Servers
#define BUFFER_SIZE 2048     // Taille du buffer pour envoi/réception

// Structure pour stocker IP et port d’un serveur
typedef struct {
    char ip[20];
    int port;
} ServerInfo;

// Liste des serveurs connus
ServerInfo servers[MAX_SERVERS] = {
    {"127.0.0.1", 6001},
    {"127.0.0.1", 6002},
    {"127.0.0.1", 6003}
};

/*********************************************************
 *  Fonction pour contacter un Library Server
 *  Renvoie 1 si succès, 0 sinon
 *********************************************************/
int contact_server(ServerInfo s, char *message, char *response) {
    int sockfd;
    struct sockaddr_in serv;

    // Création du socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return 0; // échec → serveur ignoré

    // Configuration de l'adresse du serveur
    serv.sin_family = AF_INET;
    serv.sin_port = htons(s.port);
    serv.sin_addr.s_addr = inet_addr(s.ip);

    // Connexion au serveur
    if (connect(sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        close(sockfd);
        return 0; // serveur non-répondant
    }

    // Envoi du message
    send(sockfd, message, strlen(message), 0);

    // Réception de la réponse
    int len = recv(sockfd, response, BUFFER_SIZE, 0);
    response[len] = '\0'; // terminer la chaîne

    // Fermeture du socket
    close(sockfd);
    return 1; // succès
}

/*********************************************************
 *  MAIN : boucle principale du coordinateur
 *********************************************************/
int main() {
    int sockfd, clientfd;
    struct sockaddr_in serv, client;
    char buffer[BUFFER_SIZE];
    char server_response[BUFFER_SIZE];

    // Création du socket TCP du coordinateur
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Configuration de l'adresse du coordinateur
    serv.sin_family = AF_INET;
    serv.sin_port = htons(5000);        // Port du coordinateur
    serv.sin_addr.s_addr = INADDR_ANY;  // Écoute sur toutes les interfaces

    // Bind : associe le socket à l'adresse et port
    bind(sockfd, (struct sockaddr *)&serv, sizeof(serv));

    // Écoute des connexions entrantes
    listen(sockfd, 5);
    printf("Coordinator running on port 5000...\n");

    socklen_t client_len = sizeof(client);

    // Boucle infinie pour accepter les clients
    while (1) {
        // Acceptation d'une connexion client
        clientfd = accept(sockfd, (struct sockaddr *)&client, &client_len);

        // Réception de la commande du client
        recv(clientfd, buffer, BUFFER_SIZE, 0);

        printf("\n[Client] %s\n", buffer);

        /*******************************************************
         *  Commande SEARCH
         *  Rechercher un mot-clé dans tous les Library Servers
         *******************************************************/
        if (strncmp(buffer, "search", 6) == 0) {
            char results[BUFFER_SIZE] = "";
            char aggregated[BUFFER_SIZE] = "";

            // Boucle sur tous les serveurs pour récupérer les résultats
            for (int i = 0; i < MAX_SERVERS; i++) {
                if (contact_server(servers[i], buffer, results)) {
                    strcat(aggregated, results); // concaténation des résultats
                }
            }

            // Aucun résultat trouvé ?
            if (strlen(aggregated) == 0)
                strcpy(aggregated, "Aucun résultat trouvé.\n");

            // Envoi des résultats au client
            send(clientfd, aggregated, strlen(aggregated), 0);
        }

        /*******************************************************
         *  Commande LEASE
         *  Louer un livre
         *******************************************************/
        else if (strncmp(buffer, "lease", 5) == 0) {
            for (int i = 0; i < MAX_SERVERS; i++) {
                if (contact_server(servers[i], buffer, server_response)) {
                    if (strcmp(server_response, "NOT_FOUND") != 0) {
                        send(clientfd, server_response, strlen(server_response), 0);
                        goto finished; // livre trouvé → sortir
                    }
                }
            }
            send(clientfd, "Livre introuvable.\n", 20, 0);
        }

        /*******************************************************
         *  Commande RETURN
         *  Retourner un livre loué
         *******************************************************/
        else if (strncmp(buffer, "return", 6) == 0) {
            for (int i = 0; i < MAX_SERVERS; i++) {
                if (contact_server(servers[i], buffer, server_response)) {
                    if (strcmp(server_response, "NOT_FOUND") != 0) {
                        send(clientfd, server_response, strlen(server_response), 0);
                        goto finished;
                    }
                }
            }
            send(clientfd, "Livre introuvable pour retour.\n", 32, 0);
        }

        /*******************************************************
         *  Commande LIST
         *  Lister tous les livres de tous les serveurs
         *******************************************************/
        else if (strncmp(buffer, "list", 4) == 0) {
            char aggregated[BUFFER_SIZE] = "";
            char results[BUFFER_SIZE];

            for (int i = 0; i < MAX_SERVERS; i++) {
                if (contact_server(servers[i], buffer, results)) {
                    strcat(aggregated, results);
                }
            }

            send(clientfd, aggregated, strlen(aggregated), 0);
        }

        /*******************************************************
         *  Commande STATS
         *  Récupère et affiche les statistiques de tous les serveurs
         *******************************************************/
        else if (strncmp(buffer, "stats", 5) == 0) {
            char full[BUFFER_SIZE] = "";
            char tmp[BUFFER_SIZE];

            for (int i = 0; i < MAX_SERVERS; i++) {
                if (contact_server(servers[i], "stats", tmp)) {
                    strcat(full, tmp);
                }
            }

            send(clientfd, full, strlen(full), 0);
        }

finished:
        // Fermeture de la connexion avec le client
        close(clientfd);
    }

    // Fermeture du socket coordinateur
    close(sockfd);
    return 0;
}

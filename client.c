/*********************************************************
 *  CLIENT APPLICATION
 *  ------------------
 *  - Interface CLI pour interagir avec le système
 *  - Commandes :
 *      search <keyword>, lease <book_id>, return <book_id>, list, stats, quit
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048
#define COORDINATOR_PORT 5000  // Port du coordinateur
#define COORDINATOR_IP "127.0.0.1"

/*********************************************************
 *  Fonction pour envoyer une commande au coordinateur
 *********************************************************/
void send_command(char *command) {
    int sockfd;                   // Socket pour communiquer
    struct sockaddr_in serv_addr; // Adresse du coordinateur
    char buffer[BUFFER_SIZE];     // Buffer pour recevoir la réponse

    // Création du socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {             // Vérification d'erreur
        perror("Erreur socket");
        return;
    }

    // Configuration de l'adresse du serveur
    serv_addr.sin_family = AF_INET;                      // Famille IPv4
    serv_addr.sin_port = htons(COORDINATOR_PORT);       // Port du coordinateur
    serv_addr.sin_addr.s_addr = inet_addr(COORDINATOR_IP); // IP locale

    // Connexion au coordinateur
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erreur connexion");
        close(sockfd);
        return;
    }

    // Envoi de la commande au coordinateur
    send(sockfd, command, strlen(command), 0);

    // Réception de la réponse du coordinateur
    int n = recv(sockfd, buffer, BUFFER_SIZE-1, 0);
    buffer[n] = '\0'; // Ajouter un terminator pour la chaîne

    // Affichage de la réponse à l'utilisateur
    printf("%s\n", buffer);

    // Fermeture du socket
    close(sockfd);
}

/*********************************************************
 *  MAIN : boucle principale d'interaction utilisateur
 *********************************************************/
int main() {
    char input[256]; // Buffer pour stocker la commande utilisateur

    // Affichage d'introduction et des commandes disponibles
    printf("=== Client Library System ===\n");
    printf("Commandes disponibles : search <keyword>, lease <id>, return <id>, list, stats, quit\n");

    while (1) {
        // Affichage du prompt
        printf("> ");
        fflush(stdout);

        // Lecture de la commande depuis l'entrée standard
        if (!fgets(input, sizeof(input), stdin)) break;

        // Suppression du saut de ligne final (\n)
        input[strcspn(input, "\n")] = '\0';

        // Vérification de la commande QUIT
        if (strcmp(input, "quit") == 0) {
            printf("Bye!\n");
            break;
        }

        // Envoi de la commande au coordinateur
        send_command(input);
    }

    return 0;
}

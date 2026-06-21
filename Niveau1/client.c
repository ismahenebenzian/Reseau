#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5000
#define LG_MESSAGE 1024

void afficher_grille(const char *buffer) {
    printf("%s", buffer);
}

int main() {
    int sock;
    struct sockaddr_in addr_serveur;
    char buffer[LG_MESSAGE];
    char colonne[10];
    int jeu_termine = 0;
    
    // 1. Création du socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur socket");
        exit(1);
    }
    printf("Socket créée avec succès !\n");
    
    // 2. Configuration de l'adresse du serveur
    memset(&addr_serveur, 0, sizeof(addr_serveur));
    addr_serveur.sin_family = PF_INET;
    addr_serveur.sin_port = htons(PORT);
    
    // Adresse du serveur 
    if (inet_aton("127.0.0.1", &addr_serveur.sin_addr) == 0) {
        fprintf(stderr, "Adresse du serveur invalide\n");
        close(sock);
        exit(2);
    }
    
    // 3. Connexion au serveur
    printf("Connexion au serveur %s:%d...\n", inet_ntoa(addr_serveur.sin_addr), PORT);
    if (connect(sock, (struct sockaddr*)&addr_serveur, sizeof(addr_serveur)) < 0) {
        perror("Erreur connexion");
        close(sock);
        exit(3);
    }
    printf("Connecté au serveur Puissance 4 !\n\n");
    
    // 4. Boucle principale du jeu
    while (!jeu_termine) {
        // Réinitialisation du buffer
        memset(buffer, 0, LG_MESSAGE);
        
        // Réception des données du serveur 
        int lus = read(sock, buffer, LG_MESSAGE - 1);
        if (lus <= 0) {
            if (lus == 0) {
                printf("Le serveur a fermé la connexion.\n");
            } else {
                perror("Erreur lecture");
            }
            break;
        }
        
        // Affichage de ce qu'on a reçu
        printf("%s", buffer);
        
        // Vérification des messages de fin de partie
        if (strstr(buffer, "VICTOIRE_JOUEUR")) {
            printf("\n========================================\n");
            printf("     FÉLICITATIONS ! VOUS AVEZ GAGNÉ !\n");
            printf("========================================\n");
            jeu_termine = 1;
            break;
        }
        
        if (strstr(buffer, "VICTOIRE_IA")) {
            printf("\n========================================\n");
            printf("     L'ORDINATEUR A GAGNÉ...\n");
            printf("========================================\n");
            jeu_termine = 1;
            break;
        }
        
        if (strstr(buffer, "EGALITE")) {
            printf("\n========================================\n");
            printf("          MATCH NUL !\n");
            printf("========================================\n");
            jeu_termine = 1;
            break;
        }
        
        // Vérification des messages d'erreur
        if (strstr(buffer, "ERREUR")) {
            continue;  // On redemande la colonne
        }
        
        // Si la partie n'est pas terminée, on demande une colonne
        printf("Votre choix (colonne 0-6) : ");
        
        // Lecture de l'entrée utilisateur
        if (fgets(colonne, sizeof(colonne), stdin) == NULL) {
            printf("Erreur de saisie.\n");
            break;
        }
        
        // Suppression du caractère '\n' éventuel
        size_t len = strlen(colonne);
        if (len > 0 && colonne[len-1] == '\n') {
            colonne[len-1] = '\0';
        }
        
        // Vérification simple coté client 
        int col = atoi(colonne);
        if (col < 0 || col > 6) {
            printf("Erreur : La colonne doit être comprise entre 0 et 6.\n");
            continue;
        }
        
        // Envoi de la colonne au serveur
        strcat(colonne, "\n");  // Ajout d'un retour à la ligne pour le serveur
        int ecrits = write(sock, colonne, strlen(colonne));
        if (ecrits <= 0) {
            perror("Erreur envoi");
            break;
        }
    }
    
    // 5. Fermeture du socket
    close(sock);
    printf("\nConnexion fermée. Au revoir !\n");
    
    return 0;
}

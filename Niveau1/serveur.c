#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 5000
#define LG_MESSAGE 1024

#define COLONNES 7
#define LIGNES 6
#define VIDE ' '
#define J1_JETON 'O'
#define IA_JETON 'X'

char grille[COLONNES][LIGNES];

void initialiser_grille() {
    for (int i = 0; i < COLONNES; i++)
        for (int j = 0; j < LIGNES; j++)
            grille[i][j] = VIDE;
}

void afficher_grille(char *buffer) {
    char temp[LG_MESSAGE] = "";
    char ligne[100];
    
    sprintf(temp, "\n  1   2   3   4   5   6   7  \n");
    strcat(temp, "+---+---+---+---+---+---+---+\n");
    
    for (int l = 0; l < LIGNES; l++) {
        strcat(temp, "|");
        for (int c = 0; c < COLONNES; c++) {
            sprintf(ligne, " %c |", grille[c][l]);
            strcat(temp, ligne);
        }
        strcat(temp, "\n+---+---+---+---+---+---+---+\n");
    }
    strcat(temp, "  1   2   3   4   5   6   7  \n");
    
    strcpy(buffer, temp);
}

int coup_valide(int colonne) {
    if (colonne < 0 || colonne >= COLONNES) return 0;
    return grille[colonne][0] == VIDE;
}

int jouer_coup(int colonne, char jeton) {
    for (int l = LIGNES - 1; l >= 0; l--) {
        if (grille[colonne][l] == VIDE) {
            grille[colonne][l] = jeton;
            return l;
        }
    }
    return -1;
}

int verifier_victoire(char jeton) {
    // Vérification horizontale, verticale, diagonale
    for (int c = 0; c < COLONNES; c++) {
        for (int l = 0; l < LIGNES; l++) {
            if (grille[c][l] != jeton) continue;
            
            // Horizontal droite
            if (c + 3 < COLONNES &&
                grille[c+1][l] == jeton && 
                grille[c+2][l] == jeton && 
                grille[c+3][l] == jeton) {
                return 1;
            }
            
            // Vertical bas
            if (l + 3 < LIGNES &&
                grille[c][l+1] == jeton && 
                grille[c][l+2] == jeton && 
                grille[c][l+3] == jeton) {
                return 1;
            }
            
            // Diagonale bas droite
            if (c + 3 < COLONNES && l + 3 < LIGNES &&
                grille[c+1][l+1] == jeton && 
                grille[c+2][l+2] == jeton && 
                grille[c+3][l+3] == jeton) {
                return 1;
            }
            
            // Diagonale haut droite
            if (c + 3 < COLONNES && l - 3 >= 0 &&
                grille[c+1][l-1] == jeton && 
                grille[c+2][l-2] == jeton && 
                grille[c+3][l-3] == jeton) {
                return 1;
            }
        }
    }
    return 0;
}

int grille_pleine() {
    for (int i = 0; i < COLONNES; i++)
        if (grille[i][0] == VIDE) return 0;
    return 1;
}

int ia_coup_aleatoire() {
    int colonnes_disponibles[COLONNES];
    int nb_dispo = 0;
    
    for (int i = 0; i < COLONNES; i++) {
        if (coup_valide(i)) {
            colonnes_disponibles[nb_dispo++] = i;
        }
    }
    
    if (nb_dispo == 0) return -1;
    return colonnes_disponibles[rand() % nb_dispo];
}

int main() {
    srand(time(NULL));
    
    int socket_ecoute, socket_dialogue;
    struct sockaddr_in addr_local, addr_distant;
    socklen_t addr_len = sizeof(addr_distant);
    char buffer[LG_MESSAGE];
    
    socket_ecoute = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_ecoute < 0) {
        perror("socket");
        exit(1);
    }
    
    memset(&addr_local, 0, sizeof(addr_local));
    addr_local.sin_family = PF_INET;
    addr_local.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_local.sin_port = htons(PORT);
    
    if (bind(socket_ecoute, (struct sockaddr*)&addr_local, sizeof(addr_local)) < 0) {
        perror("bind");
        exit(2);
    }
    
    if (listen(socket_ecoute, 5) < 0) {
        perror("listen");
        exit(3);
    }
    
    printf("=== Serveur Puissance 4  ===\n");
    printf("Port d'écoute : %d\n", PORT);
    printf("En attente d'un client...\n");
    
    socket_dialogue = accept(socket_ecoute, (struct sockaddr*)&addr_distant, &addr_len);
    if (socket_dialogue < 0) {
        perror("accept");
        exit(4);
    }
    
    printf("Client connecté ! Début de la partie.\n");
    printf("Vous jouez avec O, l'IA joue avec X\n\n");
    
    initialiser_grille();
    int jeu_termine = 0;
    char message[LG_MESSAGE];
    
    // Envoyer la grille initiale
    afficher_grille(message);
    write(socket_dialogue, message, strlen(message));
    
    while (!jeu_termine) {
        // Tour du joueur
        memset(buffer, 0, LG_MESSAGE);
        int lus = read(socket_dialogue, buffer, LG_MESSAGE);
        if (lus <= 0) {
            printf("Client déconnecté.\n");
            break;
        }
        
        int colonne = atoi(buffer);
        
        // Vérifie si colonne valide 
        if (colonne < 0 || colonne >= COLONNES) {
            sprintf(message, "ERREUR: Colonne invalide (0-6)\n");
            write(socket_dialogue, message, strlen(message));
            continue;
        }
        
        if (!coup_valide(colonne)) {
            sprintf(message, "ERREUR: Colonne pleine\n");
            write(socket_dialogue, message, strlen(message));
            continue;
        }
        
        // Jouer coup du joueur
        jouer_coup(colonne, J1_JETON);
        printf("Joueur joue colonne %d\n", colonne);
        
        afficher_grille(message);
        write(socket_dialogue, message, strlen(message));
        
        // Vérifier victoire joueur
        if (verifier_victoire(J1_JETON)) {
            sprintf(message, "VICTOIRE_JOUEUR\n");
            write(socket_dialogue, message, strlen(message));
            printf("Le joueur a gagné !\n");
            break;
        }
        
        // Vérifier match nul
        if (grille_pleine()) {
            sprintf(message, "EGALITE\n");
            write(socket_dialogue, message, strlen(message));
            printf("Match nul !\n");
            break;
        }
        
        // Tour de l'IA
        printf("L'IA réfléchit...\n");
        int coup_ia = ia_coup_aleatoire();
        if (coup_ia >= 0) {
            jouer_coup(coup_ia, IA_JETON);
            printf("IA joue colonne %d\n", coup_ia);
            
            afficher_grille(message);
            write(socket_dialogue, message, strlen(message));
            
            // Vérifier victoire IA
            if (verifier_victoire(IA_JETON)) {
                sprintf(message, "VICTOIRE_IA\n");
                write(socket_dialogue, message, strlen(message));
                printf("L'IA a gagné !\n");
                break;
            }
            
            // Vérifier match nul
            if (grille_pleine()) {
                sprintf(message, "EGALITE\n");
                write(socket_dialogue, message, strlen(message));
                printf("Match nul !\n");
                break;
            }
        }
    }
    
    close(socket_dialogue);
    close(socket_ecoute);
    printf("Serveur terminé.\n");
    return 0;
}

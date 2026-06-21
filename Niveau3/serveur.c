#include <stdio.h>
#include <stdlib.h> //exit
#include <string.h> //memset
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //struct sockaddr_in
#include <unistd.h> //read, write, close
#include <signal.h>
#include <sys/wait.h> //waitpid

#define PORT 5000
#define LIGNES 6
#define COLONNES 7

//LOGIQUE DU JEU

//Analyse la grille pour détecter 4 pions alignés
//Retourne : 0 (continue), 1 (J1 gagne), 2 (J2 gagne), 3 (Match nul)
int verifierVictoire(int grille[LIGNES][COLONNES]) {
    int i, j;
    //Horizontal
    for (i = 0; i < LIGNES; i++) {
        for (j = 0; j < COLONNES - 3; j++) {
            if (grille[i][j] != 0 && grille[i][j] == grille[i][j+1] && 
                grille[i][j] == grille[i][j+2] && grille[i][j] == grille[i][j+3])
                return grille[i][j]; ///id joueur gagnant
        }
    }
    //Vertical
    for (i = 0; i < LIGNES - 3; i++) {
        for (j = 0; j < COLONNES; j++) {
            if (grille[i][j] != 0 && grille[i][j] == grille[i+1][j] && 
                grille[i][j] == grille[i+2][j] && grille[i][j] == grille[i+3][j])
                return grille[i][j];
        }
    }
    //Diagonales
    for (i = 0; i < LIGNES; i++) {
        for (j = 0; j < COLONNES; j++) {
            //Diagonale descendante
            if (i <= LIGNES - 4 && j <= COLONNES - 4) {
                if (grille[i][j] != 0 && grille[i][j] == grille[i+1][j+1] && 
                    grille[i][j] == grille[i+2][j+2] && grille[i][j] == grille[i+3][j+3])
                    return grille[i][j];
            }
            //Diagonale montante
            if (i >= 3 && j <= COLONNES - 4) {
                if (grille[i][j] != 0 && grille[i][j] == grille[i-1][j+1] && 
                    grille[i][j] == grille[i-2][j+2] && grille[i][j] == grille[i-3][j+3])
                    return grille[i][j];
            }
        }
    }
    //Match nul
    for (j = 0; j < COLONNES; j++) if (grille[0][j] == 0) return 0;
    return 3;
}

int jouerCoup(int grille[LIGNES][COLONNES], int col, int joueur) {
    if (col < 0 || col >= COLONNES || grille[0][col] != 0) return 0;
    for (int i = LIGNES - 1; i >= 0; i--) {
        if (grille[i][col] == 0) {
            grille[i][col] = joueur; //placer pion
            return 1;
        }
    }
    return 0;
}

//GESTION DES PROCESSUS

void nettoyerFils(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
//wnohang -> nettoyer sans bloquer execution père

void gererPartie(int s1, int s2) {
    int grilleJeu[LIGNES][COLONNES] = {0};
    int tour = 1, fini = 0, col;
    int id1 = 1, id2 = 2;

    // Envoi initial des IDs
    if (write(s1, &id1, sizeof(int)) <= 0 || write(s2, &id2, sizeof(int)) <= 0) {
        printf("[Fils %d] Erreur lors de l'envoi des IDs. Fermeture.\n", getpid());
        close(s1); close(s2); exit(1);
    }

    while (!fini) {
        //Envoi de l'état (Grille + Tour) aux 2 joueurs
        //Vérifie le retour du write (le joueur déconnecté ?)
        if (write(s1, grilleJeu, sizeof(grilleJeu)) <= 0 || write(s1, &tour, sizeof(int)) <= 0) {
            printf("[Fils %d] Joueur 1 déconnecté.\n", getpid());
            break; 
        }
        if (write(s2, grilleJeu, sizeof(grilleJeu)) <= 0 || write(s2, &tour, sizeof(int)) <= 0) {
            printf("[Fils %d] Joueur 2 déconnecté.\n", getpid());
            break;
        }

        int socketActive = (tour == 1) ? s1 : s2;
        int socketAdverse = (tour == 1) ? s2 : s1;

        //Lecture du coup avec vérification de déconnexion
        int nbOctets = read(socketActive, &col, sizeof(int));
        
        if (nbOctets <= 0) {
            //Joueur actif parte -> prévenir l'autre avant de quitter
            printf("[Fils %d] Le joueur %d a quitté la partie.\n", getpid(), tour);
            int codeFin = -1;
            int gagnantParForfait = (tour == 1) ? 2 : 1;
            
            //Essayer d'envoyer le code de fin et le résultat à l'adversaire restant
            write(socketAdverse, grilleJeu, sizeof(grilleJeu));
            write(socketAdverse, &codeFin, sizeof(int));
            write(socketAdverse, &gagnantParForfait, sizeof(int));
            break;
        }

        //Application du coup
        if (jouerCoup(grilleJeu, col, tour)) {
            fini = verifierVictoire(grilleJeu);
            if (fini == 0) tour = (tour == 1) ? 2 : 1;
        }
    }

    //Sortie de boucle (victoire ou déconnexion)
    close(s1); 
    close(s2);
    printf("[Fils %d] Processus fils terminé proprement.\n", getpid());
    exit(0);
}

int main() {
    int ds, s1, s2;
    struct sockaddr_in adr;
    socklen_t adrLen = sizeof(adr);

    signal(SIGCHLD, nettoyerFils); //Evite processus zombies

    ds = socket(PF_INET, SOCK_STREAM, 0);
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = INADDR_ANY;
    adr.sin_port = htons(PORT);

    bind(ds, (struct sockaddr *)&adr, sizeof(adr));
    listen(ds, 10);

    printf("Serveur Niveau 3 (Fork) prêt sur le port %d\n", PORT);

    while (1) {
        s1 = accept(ds, (struct sockaddr *)&adr, &adrLen);
        printf("Joueur 1 connecté...\n");
        s2 = accept(ds, (struct sockaddr *)&adr, &adrLen);
        printf("Joueur 2 connecté \nLancement du fork\n");

        if (fork() == 0) {
            close(ds);
            gererPartie(s1, s2);
        }
        close(s1); close(s2);
    }
    return 0;
}
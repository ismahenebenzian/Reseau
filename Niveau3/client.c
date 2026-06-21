#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/socket.h>
#include <netinet/in.h> //struct sockaddr_in
#include <arpa/inet.h> //htons, inet_aton
#include <unistd.h> //read, write, close

#define LIGNES 6
#define COLONNES 7

//idem Niveau2
void afficherGrille(int grille[LIGNES][COLONNES]) {
    printf("\n  0 1 2 3 4 5 6\n");
    for (int i = 0; i < LIGNES; i++) {
        printf("| ");
        for (int j = 0; j < COLONNES; j++) {
            char c = (grille[i][j] == 0) ? '.' : (grille[i][j] == 1 ? 'X' : 'O'); //convertir entiers en char
            printf("%c ", c);
        }
        printf("|\n");
    }
}

int main() {
    int ds; //descripteur socket client
    struct sockaddr_in serv; //structure contenant coordonnées serveur
    int grille[LIGNES][COLONNES]; //matrice stocke grille
    int monId, tour, col;

    //création socket cliente
    ds = socket(PF_INET, SOCK_STREAM, 0);
    //config @ serveur
    serv.sin_family = AF_INET;
    serv.sin_port = htons(5000);

    inet_aton("10.103.113.147", &serv.sin_addr); // REMPLACER PAR L'IP DE LA VM

    //demande connexion au serveur
    if (connect(ds, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("Échec connexion");
        exit(1);
    }

    //reception id joueur (1ou2)
    read(ds, &monId, sizeof(int));
    printf("Vous êtes le Joueur %d (%c)\n", monId, (monId == 1 ? 'X' : 'O'));

    while (1) {
        //attendre etat de mise a jour
        read(ds, grille, sizeof(grille));
        read(ds, &tour, sizeof(int));

        if (tour == -1) {
            int res;
            //lecture résultat final
            read(ds, &res, sizeof(int));
            afficherGrille(grille);
            if (res == monId) printf("\nGAGNÉ !\n");
            else if (res == 3) printf("\nMATCH NUL !\n");
            else printf("\nPERDU...\n");
            break;
        }

        afficherGrille(grille);
        if (tour == monId) {
            printf("A vous de jouer (colonne 0-6) : ");
            scanf("%d", &col);
            write(ds, &col, sizeof(int));
        } else {
            printf("Attente de l'adversaire...\n");
        }
    }
    close(ds); //fermeture connexion reseau
    return 0;
}

#include <stdio.h>
#include <stdlib.h> //exit
#include <string.h> //memset
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //struct sockaddr_in
#include <unistd.h> //read, write, close

#define PORT 5000 //port de comm
//dimensions du jeu
#define LIGNES 6
#define COLONNES 7

//déclarration grille
int grille[LIGNES][COLONNES];

//remplit grille de 0
void initialiserGrille()
{
	memset(grille, 0, sizeof(grille));
}

//Envoie grille aux 2 joueurs
void envoyerGrille(int s1, int s2)
{
	write(s1, grille, sizeof(grille));
	write(s2, grille, sizeof(grille));
}

//tente inserer pion dans colonne
//parcours de bas en haut pour trouver case vide
//1 si reussi, 0 si colonne pleine ou invalide
int jouerCoup(int colonne, int joueur)
{
	if (colonne < 0 || colonne >= COLONNES) return 0;
	for (int i = LIGNES - 1; i >= 0; i--)
	{
        	if (grille[i][colonne] == 0)
		{
                grille[i][colonne] = joueur;
                return 1;
        	}
        }
    	return 0;
}

//Vérifie etat de la partie (alignement de 4 pions)
//retourne : 0 (continue) ; 1 (joueur1 gagne) ; 2 (joueur2 gagne) : 3 (match nul)
 
int verifierVictoire() {
    	int i, j;

    	// verification horizontale
    	for (i = 0; i < LIGNES; i++)
	{
        	for (j = 0; j < COLONNES - 3; j++)
		{
            		if (grille[i][j] != 0 &&
                		grille[i][j] == grille[i][j+1] &&
                		grille[i][j] == grille[i][j+2] &&
                		grille[i][j] == grille[i][j+3])
			{
                	return grille[i][j];
            		}
        	}
	}

    	// verification verticale
    	for (i = 0; i < LIGNES - 3; i++)
	{
        	for (j = 0; j < COLONNES; j++)
		{
            		if (grille[i][j] != 0 &&
                		grille[i][j] == grille[i+1][j] &&
                		grille[i][j] == grille[i+2][j] &&
                		grille[i][j] == grille[i+3][j])
			{
                		return grille[i][j];
            		}
        	}
    	}

    	//diagonale descendante
    	for (i = 0; i < LIGNES - 3; i++)
	{
        	for (j = 0; j < COLONNES - 3; j++)
		{
            		if (grille[i][j] != 0 &&
                		grille[i][j] == grille[i+1][j+1] &&
                		grille[i][j] == grille[i+2][j+2] &&
                		grille[i][j] == grille[i+3][j+3])
			{
                		return grille[i][j];
            		}
        	}
    	}

    	//diagonale montante
    	for (i = 3; i < LIGNES; i++)
	{
        	for (j = 0; j < COLONNES - 3; j++)
		{
            		if (grille[i][j] != 0 &&
                		grille[i][j] == grille[i-1][j+1] &&
                		grille[i][j] == grille[i-2][j+2] &&
                		grille[i][j] == grille[i-3][j+3])
			{
                	return grille[i][j];
            		}
        	}
    	}

    	//verification match nul (grille pleine)
    	int pleine = 1;
    	for (j = 0; j < COLONNES; j++)
	{
        	if (grille[0][j] == 0)
		{ //Si une colonne a encore de la place en haut
            		pleine = 0;
            		break;
        	}
    	}
    	if (pleine) return 3;

    	return 0; //jeu continu
}

int main()
{
	int socketEcoute, sJ1, sJ2;
	struct sockaddr_in adresse;
	int addrLen = sizeof(adresse);

	//création socket d'ecoute
	socketEcoute = socket(PF_INET, SOCK_STREAM, 0);
	//configuration adresse serveur
	adresse.sin_family = AF_INET;
	adresse.sin_addr.s_addr = INADDR_ANY; //ecoute sur toutes interfaces
 	adresse.sin_port = htons(PORT); //conversion au format réseau

	//attachement et mise en écoute
	bind(socketEcoute, (struct sockaddr *)&adresse, sizeof(adresse));
	listen(socketEcoute, 2); //2 connexions max

	printf("Serveur Puissance 4 lancé. Attente des joueurs...\n");

	//acceptation des 2 joueurs
	sJ1 = accept(socketEcoute, (struct sockaddr *)&adresse, (socklen_t*)&addrLen);
	printf("Joueur 1 connecté. Attente du Joueur 2...\n");
	int id1 = 1; write(sJ1, &id1, sizeof(int)); //informe que client = joueur1

	sJ2 = accept(socketEcoute, (struct sockaddr *)&adresse, (socklen_t*)&addrLen);
	printf("Joueur 2 connecté. Le jeu commence !\n");
	int id2 = 2; write(sJ2, &id2, sizeof(int));

	int tour = 1; //joueur1 commence
	int fini = 0; //etat de la partie
	int colonneChoisie; //recue du client actif

	//boucle tour par tour synchrone
	while (!fini)
	{
		//envoi etat actuel aux 2 clients
        	envoyerGrille(sJ1, sJ2);
        	write(sJ1, &tour, sizeof(int));
        	write(sJ2, &tour, sizeof(int));

			//determiner quelle socket doit envoyer coup
        	int socketActive = (tour == 1) ? sJ1 : sJ2;

        //attente du coup du joueur actif
		if (read(socketActive, &colonneChoisie, sizeof(int)) <= 0) 
		{
			printf("Déconnexion d'un joueur, fin de la partie \n");
			break;
		}

		//traitement du coup et arbitrage
        if (jouerCoup(colonneChoisie, tour))
		{
            fini = verifierVictoire(); //coup gagnant?
			if (fini == 0)
			{
				tour = (tour == 1) ? 2 : 1; //alternance du tour
        	}
		}
    }
	//fin partie
	envoyerGrille(sJ1, sJ2); //affichage dernier pion
	int codeFin = -1; //indiquer fin au client
	write(sJ1, &codeFin, sizeof(int));
	write(sJ2, &codeFin, sizeof(int));
	write(sJ1, &fini, sizeof(int)); //envoie 1, 2 ou 3 (gagnant ou nul)
	write(sJ2, &fini, sizeof(int));

	printf("Partie terminée \nRésultat : %d \nFermeture des sockets\n", fini);
    close(sJ1);
	close(sJ2);
	close(socketEcoute);
	
    return 0;
}

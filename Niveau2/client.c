#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/socket.h>
#include <netinet/in.h> //struct sockaddr_in
#include <arpa/inet.h> //htons, inet_aton
#include <unistd.h> //read, write, close

#define LIGNES 6
#define COLONNES 7

//affiche grille
//convertit entiers en caracteres
void afficherGrille(int grille[LIGNES][COLONNES])
{
    	printf("\n  0 1 2 3 4 5 6\n");
    	for (int i = 0; i < LIGNES; i++)
	{
        	printf("| ");
        	for (int j = 0; j < COLONNES; j++)
		{
            		char c = (grille[i][j] == 0) ? '.' : (grille[i][j] == 1 ? 'X' : 'O');
            		printf("%c ", c);
        	}
        	printf("|\n");
    	}
}

int main()
{
    int ds; //descripeur de socket
    struct sockaddr_in serv; //structure d'adresse du serveur
    int grille[LIGNES][COLONNES]; //copie locale de la grille pour affichage
    int monId, tourActuel, col; //id joueur, etat du tour et choix colonne

	//creation socket
    ds = socket(PF_INET, SOCK_STREAM, 0);
	//configuration adresse du serveur
    serv.sin_family = AF_INET;
    serv.sin_port = htons(5000); //port identique a celui serveur
   	inet_aton("10.103.113.147", &serv.sin_addr); // TODO IP VM SERVEUR ICI

	//tentative de connexion au serveur
    if (connect(ds, (struct sockaddr *)&serv, sizeof(serv)) < 0)
	{
       	perror("Connexion échouée");
     	exit(1);
    }

	//reception id attribue par serveur (1 ou 2)
    read(ds, &monId, sizeof(int));
    printf("Vous êtes le Joueur %d (%c)\n", monId, (monId==1?'X':'O'));

    while (1)
	{
		//attend la grille mise a jour par serveur
    	read(ds, grille, sizeof(grille));
		//attend info au tour de qui
    	read(ds, &tourActuel, sizeof(int));

    	afficherGrille(grille);

		//analyse du tour recu
    	if (tourActuel == monId)
	    {
        		printf("C'est votre tour. Choisissez une colonne (0-6) : ");
        		scanf("%d", &col);
        		write(ds, &col, sizeof(int));
    	}
		else 
		{
    		printf("Attente du coup de l'adversaire...\n");
    	}
		//gestion fin de partie
		if (tourActuel == -1)
		{
			int resultat;
			//lecture id gagnant
			read(ds, &resultat, sizeof(int));
			if (resultat == monId) 
			{
				printf("FELICITATION ! Vous avez gagné !\n");
			} 
			else if (resultat == 3)
			{
				printf("Match nul ! \n");
			} 
			else 
			{
				printf("Vous avez perdu ! \n");
			}
			break;
    	}
	}
	printf("Déconnexion du serveur");
    close(ds);

    return 0;
}

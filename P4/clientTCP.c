/*
 * ============================================================================
 * Fichier : clientTCP.c
 * Description : Client TCP Phase 3 - Multiservice
 * Services : Date, Liste fichiers, Contenu fichier, Durée connexion
 * Utilisation : ./clientTCP <nom_serveur> <port>
 * Exemple : ./clientTCP localhost 6000
 * Auteur : [Votre Nom]
 * Date : Décembre 2025
 * ============================================================================
 */

#include "common.h"

/* ============================================================================
 * FONCTION PRINCIPALE
 * ============================================================================ */
int main(int argc, char *argv[]) {
    
    /* ------------------------------------------------------------------------
     * DÉCLARATION DES VARIABLES
     * ------------------------------------------------------------------------ */
    
    int sockfd;
    struct sockaddr_in serveur_addr;
    struct hostent *serveur;
    int port;
    char buffer[BUFFER_SIZE];        // Buffer pour communication
    char chemin[256];                 // Pour stocker chemin/nom de fichier
    int choix;
    int resultat;

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS
     * ------------------------------------------------------------------------ */
    
    if (argc != 3) {
        printf("Usage: %s <nom_serveur> <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[2]);

    /* ------------------------------------------------------------------------
     * CRÉATION DU SOCKET ET CONNEXION (identique aux phases précédentes)
     * ------------------------------------------------------------------------ */
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur socket");
        exit(1);
    }

    serveur = gethostbyname(argv[1]);
    if (serveur == NULL) {
        printf("Erreur: serveur inconnu\n");
        exit(1);
    }

    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(port);
    memcpy(&serveur_addr.sin_addr, serveur->h_addr, serveur->h_length);

    if (connect(sockfd, (struct sockaddr *)&serveur_addr, 
                sizeof(serveur_addr)) < 0) {
        perror("Erreur connexion");
        exit(1);
    }

    printf("Connecte au serveur!\n\n");

    /* ========================================================================
     * AUTHENTIFICATION (identique aux phases précédentes)
     * ======================================================================== */
    
    printf("=== AUTHENTIFICATION ===\n");
    printf("Username: ");
    scanf("%s", buffer);
    write(sockfd, buffer, strlen(buffer) + 1);

    printf("Password: ");
    scanf("%s", buffer);
    write(sockfd, buffer, strlen(buffer) + 1);

    read(sockfd, &resultat, sizeof(int));

    if (resultat == 0) {
        printf("\nEchec authentification!\n");
        close(sockfd);
        exit(1);
    }

    printf("\nAuthentification OK!\n\n");

    /* ========================================================================
     * BOUCLE PRINCIPALE - MENU DES SERVICES
     * ======================================================================== */
    
    while (1) {
        
        /* --------------------------------------------------------------------
         * AFFICHAGE DU MENU ÉTENDU
         * -------------------------------------------------------------------- */
        
        printf("======================================\n");
        printf("           MENU DES SERVICES          \n");
        printf("======================================\n");
        printf("[1] Date et Heure du serveur\n");
        printf("[2] Liste des fichiers d'un repertoire\n");
        printf("[3] Contenu d'un fichier\n");
        printf("[4] Duree de connexion\n");
        printf("[0] Quitter\n");
        printf("======================================\n");
        printf("Votre choix: ");
        scanf("%d", &choix);
        getchar();  // Consommer le '\n' laissé par scanf

        /* --------------------------------------------------------------------
         * ENVOI DU CHOIX AU SERVEUR
         * -------------------------------------------------------------------- */
        
        write(sockfd, &choix, sizeof(int));

        if (choix == SERVICE_FIN) {
            printf("\nDeconnexion...\n");
            break;
        }

        /* --------------------------------------------------------------------
         * TRAITEMENT SELON LE SERVICE CHOISI
         * -------------------------------------------------------------------- */
        
        switch(choix) {
            
            /* ================================================================
             * SERVICE 1 : DATE ET HEURE
             * ================================================================ */
            case SERVICE_DATE:
                // Recevoir la date formatée du serveur
                memset(buffer, 0, BUFFER_SIZE);
                read(sockfd, buffer, BUFFER_SIZE);
                printf("\n>>> %s\n\n", buffer);
                break;

            /* ================================================================
             * SERVICE 2 : LISTE DES FICHIERS D'UN RÉPERTOIRE
             * ================================================================ */
            case SERVICE_LISTE_FICHIERS:
                // Demander le chemin du répertoire à l'utilisateur
                printf("Chemin du repertoire (. pour actuel): ");
                fgets(chemin, sizeof(chemin), stdin);
                
                // Enlever le '\n' à la fin
                // strcspn() trouve la position du '\n'
                chemin[strcspn(chemin, "\n")] = 0;
                
                // Envoyer le chemin au serveur
                write(sockfd, chemin, strlen(chemin) + 1);
                
                // Recevoir la liste des fichiers
                memset(buffer, 0, BUFFER_SIZE);
                read(sockfd, buffer, BUFFER_SIZE);
                printf("\n>>> LISTE DES FICHIERS:\n%s\n", buffer);
                break;

            /* ================================================================
             * SERVICE 3 : CONTENU D'UN FICHIER
             * ================================================================ */
            case SERVICE_CONTENU_FICHIER:
                // Demander le nom du fichier
                printf("Nom du fichier: ");
                fgets(chemin, sizeof(chemin), stdin);
                chemin[strcspn(chemin, "\n")] = 0;
                
                // Envoyer le nom du fichier
                write(sockfd, chemin, strlen(chemin) + 1);
                
                // Recevoir le contenu
                memset(buffer, 0, BUFFER_SIZE);
                read(sockfd, buffer, BUFFER_SIZE);
                printf("\n>>> CONTENU DU FICHIER:\n%s\n", buffer);
                break;

            /* ================================================================
             * SERVICE 4 : DURÉE DE CONNEXION
             * ================================================================ */
            case SERVICE_DUREE_CONNEXION:
                // Recevoir la durée calculée par le serveur
                memset(buffer, 0, BUFFER_SIZE);
                read(sockfd, buffer, BUFFER_SIZE);
                printf("\n>>> %s\n\n", buffer);
                break;

            default:
                printf("\nService invalide!\n\n");
        }
    }

    /* ------------------------------------------------------------------------
     * FERMETURE DE LA CONNEXION
     * ------------------------------------------------------------------------ */
    
    close(sockfd);
    printf("Au revoir!\n");
    return 0;
}

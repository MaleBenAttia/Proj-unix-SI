/*
 * ============================================================================
 * Fichier : clientTCP.c
 * Description : Client TCP Phase 1 - Monoclient avec authentification
 * Services : Date et Heure du serveur
 * Utilisation : ./clientTCP <nom_serveur> <port>
 * Exemple : ./clientTCP localhost 6000
 * Auteur : Malek Ben Attia et salim Younes
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
    
    int sockfd;                          // Descripteur du socket TCP
    struct sockaddr_in serveur_addr;     // Structure adresse du serveur
    struct hostent *serveur;             // Informations sur le serveur
    int port;                            // Numéro de port
    char buffer[BUFFER_SIZE];            // Buffer pour les échanges
    int choix;                           // Choix de l'utilisateur (menu)
    int resultat;                        // Résultat de l'authentification

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS DE LA LIGNE DE COMMANDE
     * ------------------------------------------------------------------------ */
    
    if (argc != 3) {
        printf("Usage: %s <nom_serveur> <port>\n", argv[0]);
        exit(1);
    }

    // Convertir le port (chaîne) en entier
    port = atoi(argv[2]);

    /* ------------------------------------------------------------------------
     * CRÉATION DU SOCKET TCP
     * ------------------------------------------------------------------------ */
    
    // socket(domaine, type, protocole)
    // AF_INET = IPv4
    // SOCK_STREAM = TCP (flux de données, connexion établie)
    // 0 = protocole par défaut
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur socket");
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * RÉSOLUTION DU NOM D'HÔTE EN ADRESSE IP
     * ------------------------------------------------------------------------ */
    
    // Convertir "localhost" ou "192.168.1.1" en adresse IP exploitable
    serveur = gethostbyname(argv[1]);
    if (serveur == NULL) {
        printf("Erreur: serveur inconnu\n");
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * CONFIGURATION DE L'ADRESSE DU SERVEUR
     * ------------------------------------------------------------------------ */
    
    // Initialiser la structure à zéro
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    
    // Remplir les champs de la structure
    serveur_addr.sin_family = AF_INET;                    // IPv4
    serveur_addr.sin_port = htons(port);                  // Port en network order
    memcpy(&serveur_addr.sin_addr, serveur->h_addr,      // Copier l'adresse IP
           serveur->h_length);

    /* ------------------------------------------------------------------------
     * CONNEXION AU SERVEUR (spécifique TCP)
     * ------------------------------------------------------------------------ */
    
    // connect() établit une connexion TCP avec le serveur
    // Cette fonction BLOQUE jusqu'à ce que la connexion soit établie
    // En UDP, cette étape n'existe pas !
    if (connect(sockfd, (struct sockaddr *)&serveur_addr, 
                sizeof(serveur_addr)) < 0) {
        perror("Erreur connexion");
        exit(1);
    }

    printf("Connecte au serveur!\n\n");

    /* ========================================================================
     * PHASE 1 : AUTHENTIFICATION
     * ======================================================================== */
    
    printf("=== AUTHENTIFICATION ===\n");
    
    // Demander le nom d'utilisateur
    printf("Username: ");
    scanf("%s", buffer);
    
    // Envoyer le username au serveur
    // write(descripteur, données, taille)
    // strlen(buffer) + 1 pour inclure le caractère '\0'
    write(sockfd, buffer, strlen(buffer) + 1);

    // Demander le mot de passe
    printf("Password: ");
    scanf("%s", buffer);
    
    // Envoyer le password au serveur
    write(sockfd, buffer, strlen(buffer) + 1);

    /* ------------------------------------------------------------------------
     * RÉCEPTION DU RÉSULTAT D'AUTHENTIFICATION
     * ------------------------------------------------------------------------ */
    
    // Le serveur renvoie un entier :
    // 1 = authentification réussie
    // 0 = authentification échouée
    read(sockfd, &resultat, sizeof(int));

    if (resultat == 0) {
        printf("\nEchec authentification!\n");
        close(sockfd);
        exit(1);
    }

    printf("\nAuthentification OK!\n\n");

    /* ========================================================================
     * PHASE 2 : BOUCLE DE SERVICES
     * ======================================================================== */
    
    // Boucle jusqu'à ce que l'utilisateur choisisse de quitter
    while (1) {
        
        /* --------------------------------------------------------------------
         * AFFICHAGE DU MENU
         * -------------------------------------------------------------------- */
        
        printf("=== MENU ===\n");
        printf("[1] Date et Heure\n");
        printf("[0] Quitter\n");
        printf("Choix: ");
        scanf("%d", &choix);

        /* --------------------------------------------------------------------
         * ENVOI DU CHOIX AU SERVEUR
         * -------------------------------------------------------------------- */
        
        // Envoyer le choix (entier) au serveur
        write(sockfd, &choix, sizeof(int));

        /* --------------------------------------------------------------------
         * TRAITEMENT SELON LE CHOIX
         * -------------------------------------------------------------------- */
        
        if (choix == 0) {
            // L'utilisateur veut quitter
            printf("Au revoir!\n");
            break;  // Sortir de la boucle while
        }

        if (choix == 1) {
            // Service DATE demandé
            
            // Réinitialiser le buffer à zéro
            memset(buffer, 0, BUFFER_SIZE);
            
            // Recevoir la réponse du serveur (chaîne de caractères)
            read(sockfd, buffer, BUFFER_SIZE);
            
            // Afficher la date reçue
            printf("\n%s\n\n", buffer);
        }
    }

    /* ------------------------------------------------------------------------
     * FERMETURE DE LA CONNEXION
     * ------------------------------------------------------------------------ */
    
    // Fermer le socket pour libérer les ressources
    close(sockfd);
    
    return 0;  // Fin du programme
}

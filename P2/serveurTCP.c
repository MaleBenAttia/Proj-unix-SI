/*
 * ============================================================================
 * Fichier : serveurTCP.c
 * Description : Serveur TCP Phase 1 - Monoclient avec authentification
 * Services : Date et Heure
 * Utilisation : ./serveurTCP <port>
 * Exemple : ./serveurTCP 6000
 * Compte : admin / admin123
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
    
    int sockfd;                          // Socket d'écoute (pour accept)
    int client_sockfd;                   // Socket de communication avec le client
    struct sockaddr_in serveur_addr;     // Adresse du serveur (nous)
    struct sockaddr_in client_addr;      // Adresse du client
    socklen_t client_len;                // Taille de la structure client_addr
    int port;                            // Port d'écoute
    char buffer[BUFFER_SIZE];            // Buffer de communication
    int choix;                           // Choix du service demandé
    int auth_ok;                         // Résultat de l'authentification
    time_t now;                          // Temps actuel (secondes depuis epoch)
    struct tm *timeinfo;                 // Temps décomposé (jour, mois, etc.)
    int option = 1;                      // Option pour setsockopt

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS
     * ------------------------------------------------------------------------ */
    
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    /* ------------------------------------------------------------------------
     * CRÉATION DU SOCKET TCP
     * ------------------------------------------------------------------------ */
    
    // SOCK_STREAM = TCP (différent d'UDP qui utilise SOCK_DGRAM)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur socket");
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * OPTION SO_REUSEADDR (éviter "Address already in use")
     * ------------------------------------------------------------------------ */
    
    // setsockopt(socket, niveau, nom_option, valeur, taille)
    // SOL_SOCKET = niveau socket
    // SO_REUSEADDR = permet de réutiliser immédiatement le port
    // Sans cette option, après un Ctrl+C, le port reste bloqué ~1 minute
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* ------------------------------------------------------------------------
     * CONFIGURATION DE L'ADRESSE LOCALE
     * ------------------------------------------------------------------------ */
    
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;  // Écoute sur toutes les interfaces
    serveur_addr.sin_port = htons(port);

    /* ------------------------------------------------------------------------
     * LIAISON DU SOCKET AU PORT (BIND)
     * ------------------------------------------------------------------------ */
    
    // Attache le socket au port spécifié
    if (bind(sockfd, (struct sockaddr *)&serveur_addr, 
             sizeof(serveur_addr)) < 0) {
        perror("Erreur bind");
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * MISE EN ÉCOUTE (LISTEN) - spécifique TCP
     * ------------------------------------------------------------------------ */
    
    // listen(socket, backlog)
    // backlog = nombre maximum de connexions en attente dans la file
    // Après listen(), le serveur peut accepter des connexions
    listen(sockfd, 5);

    printf("Serveur demarre sur le port %d\n", port);
    printf("Compte: admin / admin123\n");
    printf("En attente...\n\n");

    /* ------------------------------------------------------------------------
     * ACCEPTATION D'UNE CONNEXION CLIENT (ACCEPT) - spécifique TCP
     * ------------------------------------------------------------------------ */
    
    // accept(socket_ecoute, adresse_client, taille_adresse)
    // Cette fonction BLOQUE jusqu'à ce qu'un client se connecte
    // Retourne un NOUVEAU socket dédié à ce client
    // Le socket d'écoute (sockfd) reste ouvert pour d'autres clients
    client_len = sizeof(client_addr);
    client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sockfd < 0) {
        perror("Erreur accept");
        exit(1);
    }

    printf("Client connecte!\n\n");

    /* ========================================================================
     * PHASE 1 : AUTHENTIFICATION
     * ======================================================================== */
    
    char username[50], password[50];
    
    // Recevoir le nom d'utilisateur
    memset(username, 0, sizeof(username));
    read(client_sockfd, username, sizeof(username));
    printf("Username recu: %s\n", username);

    // Recevoir le mot de passe
    memset(password, 0, sizeof(password));
    read(client_sockfd, password, sizeof(password));
    printf("Password recu: %s\n", password);

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES IDENTIFIANTS
     * ⚠️ VOUS POUVEZ MODIFIER LE MOT DE PASSE ICI
     * ------------------------------------------------------------------------ */
    
    // strcmp() retourne 0 si les chaînes sont identiques
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin123") == 0) {
        auth_ok = 1;  // Authentification réussie
        printf("Auth OK\n\n");
    } else {
        auth_ok = 0;  // Authentification échouée
        printf("Auth ECHEC\n\n");
    }

    // Envoyer le résultat au client
    write(client_sockfd, &auth_ok, sizeof(int));

    // Si l'authentification a échoué, fermer et terminer
    if (auth_ok == 0) {
        close(client_sockfd);
        close(sockfd);
        exit(1);
    }

    /* ========================================================================
     * PHASE 2 : BOUCLE DE SERVICE
     * ======================================================================== */
    
    while (1) {
        
        /* --------------------------------------------------------------------
         * RÉCEPTION DU CHOIX DU CLIENT
         * -------------------------------------------------------------------- */
        
        // Recevoir le choix du service
        read(client_sockfd, &choix, sizeof(int));
        printf("Choix: %d\n", choix);

        /* --------------------------------------------------------------------
         * TRAITEMENT SELON LE CHOIX
         * -------------------------------------------------------------------- */
        
        if (choix == 0) {
            // Le client veut se déconnecter
            printf("Client deconnecte\n");
            break;  // Sortir de la boucle
        }

        if (choix == 1) {
            /* ----------------------------------------------------------------
             * SERVICE DATE ET HEURE
             * ---------------------------------------------------------------- */
            
            // Étape 1 : Obtenir le temps actuel en secondes depuis epoch
            time(&now);
            
            // Étape 2 : Convertir en structure tm (temps local)
            timeinfo = localtime(&now);
            
            // Étape 3 : Formater la date en chaîne de caractères
            // Format : "Date: jour/mois/année - Heure: heure:minute:seconde"
            // %d = jour (01-31)
            // %m = mois (01-12)
            // %Y = année (4 chiffres)
            // %H = heure 24h (00-23)
            // %M = minutes (00-59)
            // %S = secondes (00-59)
            strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", 
                     timeinfo);
            
            printf("Envoi: %s\n\n", buffer);
            
            // Étape 4 : Envoyer la chaîne formatée au client
            write(client_sockfd, buffer, strlen(buffer) + 1);
        }
    }

    /* ------------------------------------------------------------------------
     * FERMETURE DES CONNEXIONS
     * ------------------------------------------------------------------------ */
    
    // Fermer le socket client
    close(client_sockfd);
    
    // Fermer le socket d'écoute
    close(sockfd);
    
    return 0;
}

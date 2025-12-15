/*
 * ============================================================================
 * Fichier : serveurTCP.c
 * Description : Serveur TCP Phase 2 - MULTICLIENT avec fork()
 * Chaque client est géré par un processus fils indépendant
 * Utilisation : ./serveurTCP <port>
 * Exemple : ./serveurTCP 6000
 * Auteur : Malek Ben Attia et Salim Younes
 * Date : Décembre 2025
 * ============================================================================
 */

#include "common.h"

/* ============================================================================
 * GESTIONNAIRE DE SIGNAL SIGCHLD
 * ============================================================================
 * Rôle : Récupérer les processus fils terminés (éviter les zombies)
 * 
 * Un processus zombie est un processus fils terminé mais dont le père
 * n'a pas encore récupéré le code de sortie avec wait() ou waitpid().
 * Le zombie reste dans la table des processus et consomme des ressources.
 * ============================================================================
 */
void gestionnaire_sigchld(int sig) {
    // waitpid(-1, NULL, WNOHANG)
    // -1 = attendre n'importe quel fils
    // NULL = on ne récupère pas le statut de sortie
    // WNOHANG = non bloquant (retourne immédiatement si pas de fils terminé)
    // 
    // Boucle while : tant qu'il y a des fils zombies, les récupérer tous
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* ============================================================================
 * FONCTION POUR TRAITER UN CLIENT
 * ============================================================================
 * Cette fonction est exécutée par chaque processus fils
 * Chaque fils gère UN SEUL client de A à Z
 * ============================================================================
 */
void traiter_client(int client_sockfd) {
    
    /* ------------------------------------------------------------------------
     * VARIABLES LOCALES DU PROCESSUS FILS
     * ------------------------------------------------------------------------ */
    
    char buffer[BUFFER_SIZE];
    int choix;
    int auth_ok;
    time_t now;
    struct tm *timeinfo;
    char username[50], password[50];

    // Afficher le PID du processus fils (pour déboguer)
    printf("[PID %d] Nouveau client\n", getpid());

    /* ========================================================================
     * AUTHENTIFICATION
     * ======================================================================== */
    
    memset(username, 0, sizeof(username));
    read(client_sockfd, username, sizeof(username));

    memset(password, 0, sizeof(password));
    read(client_sockfd, password, sizeof(password));

    printf("[PID %d] Username: %s, Password: %s\n", getpid(), username, password);

    // Vérifier les identifiants (MODIFIABLE ICI)
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin123") == 0) {
        auth_ok = 1;
        printf("[PID %d] Auth OK\n", getpid());
    } else {
        auth_ok = 0;
        printf("[PID %d] Auth ECHEC\n", getpid());
    }

    write(client_sockfd, &auth_ok, sizeof(int));

    if (auth_ok == 0) {
        close(client_sockfd);
        exit(0);  // Le fils se termine
    }

    /* ========================================================================
     * BOUCLE DE SERVICE
     * ======================================================================== */
    
    while (1) {
        // Recevoir le choix
        int n = read(client_sockfd, &choix, sizeof(int));
        
        // Si read retourne 0 ou -1, le client s'est déconnecté
        if (n <= 0) {
            printf("[PID %d] Client deconnecte\n", getpid());
            break;
        }

        printf("[PID %d] Choix: %d\n", getpid(), choix);

        if (choix == 0) {
            printf("[PID %d] Client quitte\n", getpid());
            break;
        }

        if (choix == 1) {
            // Service DATE
            time(&now);
            timeinfo = localtime(&now);
            strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", 
                     timeinfo);
            
            printf("[PID %d] Envoi: %s\n", getpid(), buffer);
            write(client_sockfd, buffer, strlen(buffer) + 1);
        }
    }

    /* ------------------------------------------------------------------------
     * FIN DU TRAITEMENT POUR CE CLIENT
     * ------------------------------------------------------------------------ */
    
    close(client_sockfd);
    printf("[PID %d] Fin du processus\n", getpid());
    
    // exit(0) termine le processus fils
    // Le fils disparaît complètement de la mémoire
    exit(0);
}

/* ============================================================================
 * FONCTION PRINCIPALE (PROCESSUS PÈRE)
 * ============================================================================ */
int main(int argc, char *argv[]) {
    
    /* ------------------------------------------------------------------------
     * DÉCLARATION DES VARIABLES
     * ------------------------------------------------------------------------ */
    
    int sockfd, client_sockfd;
    struct sockaddr_in serveur_addr, client_addr;
    socklen_t client_len;
    int port;
    int option = 1;
    pid_t pid;  // Type spécial pour les ID de processus

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS
     * ------------------------------------------------------------------------ */
    
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    /* ------------------------------------------------------------------------
     * INSTALLATION DU GESTIONNAIRE DE SIGNAL
     * ------------------------------------------------------------------------ */
    
    // signal(numéro_signal, fonction_gestionnaire)
    // SIGCHLD est envoyé au père quand un fils se termine
    // On installe notre fonction pour récupérer les zombies
    signal(SIGCHLD, gestionnaire_sigchld);

    /* ------------------------------------------------------------------------
     * CRÉATION ET CONFIGURATION DU SOCKET
     * ------------------------------------------------------------------------ */
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur socket");
        exit(1);
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    serveur_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serveur_addr, 
             sizeof(serveur_addr)) < 0) {
        perror("Erreur bind");
        exit(1);
    }

    listen(sockfd, 5);

    printf("========================================\n");
    printf("Serveur MULTICLIENT demarre (port %d)\n", port);
    printf("Compte: admin / admin123\n");
    printf("========================================\n");
    printf("En attente de clients...\n\n");

    /* ========================================================================
     * BOUCLE PRINCIPALE - ACCEPTER PLUSIEURS CLIENTS
     * ======================================================================== */
    
    while (1) {
        
        /* --------------------------------------------------------------------
         * ATTENDRE UN NOUVEAU CLIENT
         * -------------------------------------------------------------------- */
        
        client_len = sizeof(client_addr);
        client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, 
                               &client_len);
        
        if (client_sockfd < 0) {
            perror("Erreur accept");
            continue;  // Continuer à attendre d'autres clients
        }

        printf("\n[SERVEUR] Nouveau client connecte!\n");

        /* --------------------------------------------------------------------
         * CRÉER UN PROCESSUS FILS POUR CE CLIENT
         * -------------------------------------------------------------------- */
        
        // fork() duplique le processus courant
        // Retourne :
        // - 0 dans le fils
        // - PID du fils dans le père
        // - -1 si erreur
        pid = fork();

        if (pid < 0) {
            // Erreur lors du fork
            perror("Erreur fork");
            close(client_sockfd);
            continue;
        }

        if (pid == 0) {
            /* ================================================================
             * CODE EXÉCUTÉ PAR LE PROCESSUS FILS
             * ================================================================ */
            
            // Le fils n'a pas besoin du socket d'écoute
            close(sockfd);
            
            // Traiter ce client jusqu'à déconnexion
            traiter_client(client_sockfd);
            
            // traiter_client() fait exit(0), on n'arrive jamais ici
            
        } else {
            /* ================================================================
             * CODE EXÉCUTÉ PAR LE PROCESSUS PÈRE
             * ================================================================ */
            
            // Le père n'a pas besoin du socket client
            // C'est le fils qui communique avec le client
            close(client_sockfd);
            
            printf("[SERVEUR] Client gere par le processus %d\n", pid);
            
            // Le père retourne immédiatement à accept() pour attendre
            // le prochain client, pendant que le fils traite ce client
        }
    }

    /* ------------------------------------------------------------------------
     * FERMETURE (code jamais atteint car boucle infinie)
     * ------------------------------------------------------------------------ */
    
    close(sockfd);
    return 0;
}

/* ============================================================================
 * EXPLICATION DU FONCTIONNEMENT MULTICLIENT
 * ============================================================================
 * 
 * 1. Le père attend un client avec accept()
 * 2. Un client se connecte → accept() retourne un socket client
 * 3. Le père fait fork() pour créer un fils
 * 4. Le fils prend en charge le client et communique avec lui
 * 5. Le père retourne immédiatement à accept() pour le prochain client
 * 6. Plusieurs clients peuvent être connectés simultanément
 * 
 * MÉMOIRE :
 * - Chaque processus fils a sa propre mémoire (variables séparées)
 * - Les fils ne partagent PAS de variables avec le père
 * - Chaque fils a son propre client_sockfd
 * 
 * ZOMBIES :
 * - Quand un fils termine, il devient zombie
 * - Le gestionnaire SIGCHLD récupère automatiquement les zombies
 * - Cela évite d'encombrer la table des processus
 * ============================================================================
 */

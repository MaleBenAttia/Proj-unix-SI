/*
 * ============================================================================
 * Fichier : serveurTCP.c
 * Description : Serveur TCP Phase 3 - Multiclient/Multiservice
 * Services : Date, Liste fichiers, Contenu fichier, Durée connexion
 * Utilisation : ./serveurTCP <port>
 * Exemple : ./serveurTCP 6000
 * Compte : admin / admin123
 * Auteur : [Votre Nom]
 * Date : Décembre 2025
 * ============================================================================
 */

#include "common.h"

/* ============================================================================
 * GESTIONNAIRE DE SIGNAL SIGCHLD
 * ============================================================================ */
void gestionnaire_sigchld(int sig) {
    // Récupérer tous les processus fils terminés (éviter les zombies)
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* ============================================================================
 * SERVICE 1 : DATE ET HEURE SYSTÈME
 * ============================================================================
 * Envoie la date et l'heure actuelles du serveur au format :
 * "Date: JJ/MM/AAAA - Heure: HH:MM:SS"
 * ============================================================================ */
void service_date(int client_sockfd) {
    char buffer[BUFFER_SIZE];
    time_t now;
    struct tm *timeinfo;
    
    // Obtenir le temps actuel (secondes depuis 01/01/1970)
    time(&now);
    
    // Convertir en structure tm (temps local avec jour, mois, etc.)
    timeinfo = localtime(&now);
    
    // Formater en chaîne lisible
    // %d = jour (01-31)
    // %m = mois (01-12)
    // %Y = année (4 chiffres)
    // %H = heure 24h (00-23)
    // %M = minutes (00-59)
    // %S = secondes (00-59)
    strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", timeinfo);
    
    // Envoyer la chaîne formatée au client
    write(client_sockfd, buffer, strlen(buffer) + 1);
    printf("[PID %d] Service DATE envoye\n", getpid());
}

/* ============================================================================
 * SERVICE 2 : LISTE DES FICHIERS D'UN RÉPERTOIRE
 * ============================================================================
 * Lit tous les fichiers d'un répertoire et envoie la liste au client
 * Utilise les fonctions opendir(), readdir(), closedir()
 * ============================================================================ */
void service_liste_fichiers(int client_sockfd) {
    char chemin[256];
    char buffer[BUFFER_SIZE];
    DIR *dir;                   // Pointeur vers le flux répertoire
    struct dirent *entry;       // Structure pour chaque entrée du répertoire
    
    /* ------------------------------------------------------------------------
     * RÉCEPTION DU CHEMIN DU RÉPERTOIRE
     * ------------------------------------------------------------------------ */
    
    memset(chemin, 0, sizeof(chemin));
    read(client_sockfd, chemin, sizeof(chemin));
    
    printf("[PID %d] Liste fichiers de: %s\n", getpid(), chemin);
    
    /* ------------------------------------------------------------------------
     * OUVERTURE DU RÉPERTOIRE
     * ------------------------------------------------------------------------ */
    
    // opendir() retourne un pointeur DIR* ou NULL si erreur
    // Causes d'erreur : répertoire inexistant, pas de permissions, etc.
    dir = opendir(chemin);
    if (dir == NULL) {
        sprintf(buffer, "Erreur: Impossible d'ouvrir le repertoire");
        write(client_sockfd, buffer, strlen(buffer) + 1);
        return;
    }
    
    /* ------------------------------------------------------------------------
     * LECTURE DE TOUTES LES ENTRÉES
     * ------------------------------------------------------------------------ */
    
    memset(buffer, 0, BUFFER_SIZE);
    
    // readdir() retourne un pointeur vers l'entrée suivante
    // Retourne NULL quand il n'y a plus d'entrées
    // Boucle : lire toutes les entrées jusqu'à la fin
    while ((entry = readdir(dir)) != NULL) {
        // entry->d_name contient le nom du fichier/répertoire
        strcat(buffer, entry->d_name);  // Ajouter le nom au buffer
        strcat(buffer, "\n");            // Ajouter un retour à la ligne
    }
    
    // Fermer le répertoire
    closedir(dir);
    
    /* ------------------------------------------------------------------------
     * ENVOI DE LA LISTE AU CLIENT
     * ------------------------------------------------------------------------ */
    
    write(client_sockfd, buffer, strlen(buffer) + 1);
    printf("[PID %d] Liste envoyee\n", getpid());
}

/* ============================================================================
 * SERVICE 3 : CONTENU D'UN FICHIER
 * ============================================================================
 * Lit le contenu d'un fichier texte et l'envoie au client
 * Utilise les fonctions fopen(), fread(), fclose()
 * ============================================================================ */
void service_contenu_fichier(int client_sockfd) {
    char nom_fichier[256];
    char buffer[BUFFER_SIZE];
    FILE *file;                  // Pointeur vers le flux fichier
    int n;                       // Nombre d'octets lus
    
    /* ------------------------------------------------------------------------
     * RÉCEPTION DU NOM DU FICHIER
     * ------------------------------------------------------------------------ */
    
    memset(nom_fichier, 0, sizeof(nom_fichier));
    read(client_sockfd, nom_fichier, sizeof(nom_fichier));
    
    printf("[PID %d] Contenu fichier: %s\n", getpid(), nom_fichier);
    
    /* ------------------------------------------------------------------------
     * OUVERTURE DU FICHIER EN LECTURE
     * ------------------------------------------------------------------------ */
    
    // fopen(chemin, mode)
    // "r" = read (lecture)
    // Retourne un pointeur FILE* ou NULL si erreur
    file = fopen(nom_fichier, "r");
    if (file == NULL) {
        sprintf(buffer, "Erreur: Impossible d'ouvrir le fichier");
        write(client_sockfd, buffer, strlen(buffer) + 1);
        return;
    }
    
    /* ------------------------------------------------------------------------
     * LECTURE DU CONTENU
     * ------------------------------------------------------------------------ */
    
    memset(buffer, 0, BUFFER_SIZE);
    
    // fread(buffer, taille_element, nombre_elements, fichier)
    // Ici : lire BUFFER_SIZE-1 octets de 1 octet chacun
    // On garde 1 octet pour le '\0' final
    // Retourne : nombre d'éléments lus
    n = fread(buffer, 1, BUFFER_SIZE - 1, file);
    
    // Ajouter le '\0' de fin de chaîne
    buffer[n] = '\0';
    
    // Fermer le fichier
    fclose(file);
    
    /* ------------------------------------------------------------------------
     * ENVOI DU CONTENU AU CLIENT
     * ------------------------------------------------------------------------ */
    
    write(client_sockfd, buffer, strlen(buffer) + 1);
    printf("[PID %d] Contenu envoye (%d octets)\n", getpid(), n);
}

/* ============================================================================
 * SERVICE 4 : DURÉE DE CONNEXION
 * ============================================================================
 * Calcule le temps écoulé depuis la connexion du client
 * Utilise la fonction difftime() pour calculer la différence
 * ============================================================================ */
void service_duree_connexion(int client_sockfd, time_t debut) {
    char buffer[BUFFER_SIZE];
    time_t maintenant;
    int duree_sec, minutes, secondes;
    
    /* ------------------------------------------------------------------------
     * CALCUL DE LA DURÉE
     * ------------------------------------------------------------------------ */
    
    // Obtenir l'heure actuelle
    time(&maintenant);
    
    // difftime(temps_final, temps_initial)
    // Retourne la différence en secondes (type double)
    // On cast en int car on n'a pas besoin de précision sub-seconde
    duree_sec = (int)difftime(maintenant, debut);
    
    // Convertir en minutes et secondes
    minutes = duree_sec / 60;        // Division entière
    secondes = duree_sec % 60;       // Reste de la division (modulo)
    
    /* ------------------------------------------------------------------------
     * FORMATAGE ET ENVOI
     * ------------------------------------------------------------------------ */
    
    // sprintf() = printf() dans un buffer
    sprintf(buffer, "Duree de connexion: %d minute(s) et %d seconde(s)", 
            minutes, secondes);
    
    write(client_sockfd, buffer, strlen(buffer) + 1);
    printf("[PID %d] Duree connexion envoyee: %d:%d\n", getpid(), minutes, secondes);
}

/* ============================================================================
 * FONCTION POUR TRAITER UN CLIENT
 * ============================================================================
 * Exécutée par chaque processus fils
 * Gère l'authentification et la boucle de services
 * ============================================================================ */
void traiter_client(int client_sockfd) {
    char username[50], password[50];
    int choix;
    int auth_ok;
    time_t debut_connexion;  // Heure de début pour le service 4
    
    printf("[PID %d] Nouveau client\n", getpid());
    
    /* ========================================================================
     * ENREGISTRER L'HEURE DE CONNEXION
     * ======================================================================== */
    
    // Sauvegarder l'heure actuelle pour calculer la durée plus tard
    time(&debut_connexion);

    /* ========================================================================
     * AUTHENTIFICATION
     * ======================================================================== */
    
    memset(username, 0, sizeof(username));
    read(client_sockfd, username, sizeof(username));

    memset(password, 0, sizeof(password));
    read(client_sockfd, password, sizeof(password));

    printf("[PID %d] Username: %s, Password: %s\n", getpid(), username, password);

    // ⚠️ MODIFIEZ LE MOT DE PASSE ICI
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
        exit(0);
    }

    /* ========================================================================
     * BOUCLE DE TRAITEMENT DES SERVICES
     * ======================================================================== */
    
    while (1) {
        
        /* --------------------------------------------------------------------
         * RÉCEPTION DU CHOIX DU CLIENT
         * -------------------------------------------------------------------- */
        
        int n = read(client_sockfd, &choix, sizeof(int));
        if (n <= 0) {
            printf("[PID %d] Client deconnecte\n", getpid());
            break;
        }

        printf("[PID %d] Service demande: %d\n", getpid(), choix);

        /* --------------------------------------------------------------------
         * DISPATCH VERS LE SERVICE APPROPRIÉ
         * -------------------------------------------------------------------- */
        
        // switch/case : exécuter du code différent selon la valeur de choix
        switch(choix) {
            case SERVICE_FIN:
                // Client veut se déconnecter
                printf("[PID %d] Client quitte\n", getpid());
                goto fin;  // goto pour sortir de la boucle et du switch
                
            case SERVICE_DATE:
                // Appeler la fonction service_date()
                service_date(client_sockfd);
                break;  // Sortir du switch, continuer la boucle
                
            case SERVICE_LISTE_FICHIERS:
                service_liste_fichiers(client_sockfd);
                break;
                
            case SERVICE_CONTENU_FICHIER:
                service_contenu_fichier(client_sockfd);
                break;
                
            case SERVICE_DUREE_CONNEXION:
                // Passer l'heure de début enregistrée au début
                service_duree_connexion(client_sockfd, debut_connexion);
                break;
                
            default:
                // Choix invalide
                printf("[PID %d] Service invalide\n", getpid());
        }
    }

    /* ------------------------------------------------------------------------
     * FIN DU TRAITEMENT
     * ------------------------------------------------------------------------ */
    
fin:  // Label pour le goto
    close(client_sockfd);
    printf("[PID %d] Fin du processus\n", getpid());
    exit(0);  // Terminer le processus fils
}

/* ============================================================================
 * FONCTION PRINCIPALE (PROCESSUS PÈRE)
 * ============================================================================ */
int main(int argc, char *argv[]) {
    
    int sockfd, client_sockfd;
    struct sockaddr_in serveur_addr, client_addr;
    socklen_t client_len;
    int port;
    int option = 1;
    pid_t pid;

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

    /* ------------------------------------------------------------------------
     * AFFICHAGE DES INFORMATIONS DU SERVEUR
     * ------------------------------------------------------------------------ */
    
    printf("===========================================\n");
    printf("  SERVEUR MULTISERVICE (port %d)          \n", port);
    printf("===========================================\n");
    printf("Compte: admin / admin123\n");
    printf("Services disponibles:\n");
    printf("  [1] Date et Heure\n");
    printf("  [2] Liste des fichiers\n");
    printf("  [3] Contenu d'un fichier\n");
    printf("  [4] Duree de connexion\n");
    printf("===========================================\n");
    printf("En attente de clients...\n\n");

    /* ========================================================================
     * BOUCLE PRINCIPALE - ACCEPTER ET FORKER
     * ======================================================================== */
    
    while (1) {
        client_len = sizeof(client_addr);
        client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, 
                               &client_len);
        
        if (client_sockfd < 0) {
            perror("Erreur accept");
            continue;
        }

        printf("\n[SERVEUR] Nouveau client connecte!\n");

        // Créer un processus fils pour ce client
        pid = fork();

        if (pid < 0) {
            perror("Erreur fork");
            close(client_sockfd);
            continue;
        }

        if (pid == 0) {
            // PROCESSUS FILS
            close(sockfd);
            traiter_client(client_sockfd);
            // traiter_client() fait exit(0)
        } else {
            // PROCESSUS PÈRE
            close(client_sockfd);
            printf("[SERVEUR] Client gere par processus %d\n", pid);
        }
    }

    close(sockfd);
    return 0;
}

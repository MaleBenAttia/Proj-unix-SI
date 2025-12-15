/*
 * ============================================================================
 * Fichier : serveurTCP.c
 * Description : Serveur TCP Multiservice Multi-Port avec Threads
 * Chaque service écoute sur un port différent et tourne dans son propre thread
 * Compilation : gcc serveurTCP.c -o serveurTCP -lpthread
 * Utilisation : ./serveurTCP <username> <password>
 * Exemple : ./serveurTCP admin admin123
 * Auteur : Malek Ben Attia et Salim younes
 
 * Date : Décembre 2025
 * ============================================================================
 */

#include "common.h"
#include <pthread.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================
 * Ces variables sont partagées entre tous les threads
 * Attention : l'accès concurrent doit être protégé par des mutex
 * ============================================================================ */

// Identifiants pour l'authentification
static char global_username[50];
static char global_password[50];

// Flag pour arrêter proprement tous les threads
static volatile int serveur_actif = 1;

// Mutex pour protéger les affichages (éviter que les printf se mélangent)
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ----------------------------------------------------------------------------
 * SOCKETS GLOBAUX - Un par service
 * ---------------------------------------------------------------------------- */
static int sockfd_auth;      // Socket pour le service d'authentification
static int sockfd_date;      // Socket pour le service date/heure
static int sockfd_liste;     // Socket pour le service liste de fichiers
static int sockfd_contenu;   // Socket pour le service contenu de fichier
static int sockfd_duree;     // Socket pour le service durée de connexion

/* ============================================================================
 * FONCTION : afficher_log
 * ============================================================================
 * Rôle : Affichage thread-safe (protégé par mutex)
 * 
 * Paramètres :
 *   - message : le texte à afficher
 * 
 * Pourquoi un mutex ?
 *   Sans mutex, si 2 threads font printf() en même temps, les caractères
 *   peuvent se mélanger : "Hello" + "World" → "HWeolrllod"
 *   Le mutex garantit qu'un seul thread écrit à la fois
 * ============================================================================ */
void afficher_log(const char *message) {
    pthread_mutex_lock(&log_mutex);      // Verrouiller
    printf("%s", message);               // Afficher
    fflush(stdout);                      // Forcer l'affichage immédiat
    pthread_mutex_unlock(&log_mutex);    // Déverrouiller
}

/* ============================================================================
 * FONCTION : creer_socket_serveur
 * ============================================================================
 * Rôle : Créer et configurer un socket serveur TCP
 * 
 * Paramètres :
 *   - port : numéro de port sur lequel écouter
 * 
 * Retour :
 *   - Le descripteur du socket créé (>= 0)
 *   - -1 en cas d'erreur
 * 
 * Étapes :
 *   1. socket()  → Créer le socket
 *   2. setsockopt() → Permettre la réutilisation du port (SO_REUSEADDR)
 *   3. bind()    → Attacher le socket au port
 *   4. listen()  → Passer en mode écoute
 * ============================================================================ */
int creer_socket_serveur(int port) {
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 1 : Créer le socket
     * AF_INET = IPv4
     * SOCK_STREAM = TCP (connexion fiable)
     * 0 = protocole par défaut
     * ------------------------------------------------------------------------ */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 2 : Option SO_REUSEADDR
     * Permet de redémarrer le serveur immédiatement après l'arrêt
     * Sans cela, on aurait "Address already in use" pendant ~60 secondes
     * ------------------------------------------------------------------------ */
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 3 : Préparer l'adresse du serveur
     * ------------------------------------------------------------------------ */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));           // Initialiser à zéro
    addr.sin_family = AF_INET;                // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;        // Écouter sur toutes les interfaces
    addr.sin_port = htons(port);              // Convertir le port en big-endian
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 4 : Attacher le socket au port
     * ------------------------------------------------------------------------ */
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 5 : Passer en mode écoute
     * La file d'attente peut contenir jusqu'à 5 connexions en attente
     * ------------------------------------------------------------------------ */
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/* ============================================================================
 * THREAD : SERVICE D'AUTHENTIFICATION
 * ============================================================================
 * Port : PORT_AUTH
 * Rôle : Vérifier les identifiants username/password
 * 
 * Protocole :
 *   Client → Serveur : username (50 octets)
 *   Client → Serveur : password (50 octets)
 *   Serveur → Client : résultat (int) AUTH_SUCCESS ou AUTH_FAILURE
 * ============================================================================ */
void* thread_service_auth(void* arg) {
    char log_msg[256];
    
    /* ------------------------------------------------------------------------
     * BOUCLE PRINCIPALE : Accepter les clients en continu
     * ------------------------------------------------------------------------ */
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /* --------------------------------------------------------------------
         * Attendre un client
         * accept() bloque jusqu'à ce qu'un client se connecte
         * -------------------------------------------------------------------- */
        int client_sock = accept(sockfd_auth, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept auth");
            }
            continue;  // Continuer si erreur non critique
        }
        
        sprintf(log_msg, "🔐 [AUTH] Nouvelle demande d'authentification\n");
        afficher_log(log_msg);
        
        /* --------------------------------------------------------------------
         * Recevoir les identifiants
         * -------------------------------------------------------------------- */
        char username[50], password[50];
        
        memset(username, 0, sizeof(username));
        read(client_sock, username, sizeof(username));
        
        memset(password, 0, sizeof(password));
        read(client_sock, password, sizeof(password));
        
        /* --------------------------------------------------------------------
         * Vérifier les identifiants
         * -------------------------------------------------------------------- */
        int auth_ok;
        if (strcmp(username, global_username) == 0 && 
            strcmp(password, global_password) == 0) {
            auth_ok = AUTH_SUCCESS;
            sprintf(log_msg, "   ✅ Authentification OK pour: %s\n", username);
        } else {
            auth_ok = AUTH_FAILURE;
            sprintf(log_msg, "   ❌ Authentification ÉCHEC pour: %s\n", username);
        }
        afficher_log(log_msg);
        
        /* --------------------------------------------------------------------
         * Envoyer le résultat au client
         * -------------------------------------------------------------------- */
        write(client_sock, &auth_ok, sizeof(int));
        close(client_sock);
    }
    
    return NULL;
}

/* ============================================================================
 * THREAD : SERVICE DATE ET HEURE
 * ============================================================================
 * Port : PORT_DATE
 * Rôle : Retourner la date et l'heure actuelles
 * 
 * Protocole :
 *   Serveur → Client : chaîne formatée "Date: JJ/MM/AAAA - Heure: HH:MM:SS"
 * ============================================================================ */
void* thread_service_date(void* arg) {
    char log_msg[256];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /* --------------------------------------------------------------------
         * Attendre un client
         * -------------------------------------------------------------------- */
        int client_sock = accept(sockfd_date, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept date");
            }
            continue;
        }
        
        sprintf(log_msg, "📅 [DATE] Demande de date/heure\n");
        afficher_log(log_msg);
        
        /* --------------------------------------------------------------------
         * Obtenir la date/heure actuelle
         * -------------------------------------------------------------------- */
        char buffer[BUFFER_SIZE];
        time_t now;
        struct tm *timeinfo;
        
        time(&now);                    // Obtenir le timestamp Unix
        timeinfo = localtime(&now);    // Convertir en heure locale
        
        // Formater la date/heure
        strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", timeinfo);
        
        /* --------------------------------------------------------------------
         * Envoyer au client
         * -------------------------------------------------------------------- */
        write(client_sock, buffer, strlen(buffer) + 1);
        afficher_log("   ✅ Date/Heure envoyée\n");
        
        close(client_sock);
    }
    
    return NULL;
}

/* ============================================================================
 * THREAD : SERVICE LISTE DES FICHIERS
 * ============================================================================
 * Port : PORT_LISTE
 * Rôle : Lister le contenu d'un répertoire
 * 
 * Protocole :
 *   Client → Serveur : chemin du répertoire (256 octets)
 *   Serveur → Client : liste des fichiers séparés par '\n'
 * ============================================================================ */
void* thread_service_liste(void* arg) {
    char log_msg[512];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /* --------------------------------------------------------------------
         * Attendre un client
         * -------------------------------------------------------------------- */
        int client_sock = accept(sockfd_liste, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept liste");
            }
            continue;
        }
        
        /* --------------------------------------------------------------------
         * Recevoir le chemin du répertoire
         * -------------------------------------------------------------------- */
        char chemin[256];
        char buffer[BUFFER_SIZE];
        
        memset(chemin, 0, sizeof(chemin));
        read(client_sock, chemin, sizeof(chemin));
        
        snprintf(log_msg, sizeof(log_msg), "📂 [LISTE] Demande pour: %s\n", chemin);
        afficher_log(log_msg);
        
        /* --------------------------------------------------------------------
         * Lire le contenu du répertoire
         * -------------------------------------------------------------------- */
        DIR *dir = opendir(chemin);
        if (dir == NULL) {
            sprintf(buffer, "Erreur: Impossible d'ouvrir le répertoire");
            afficher_log("   ❌ Erreur d'ouverture\n");
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            struct dirent *entry;
            
            // Parcourir toutes les entrées du répertoire
            while ((entry = readdir(dir)) != NULL) {
                strcat(buffer, entry->d_name);
                strcat(buffer, "\n");
            }
            closedir(dir);
            afficher_log("   ✅ Liste envoyée\n");
        }
        
        /* --------------------------------------------------------------------
         * Envoyer la liste au client
         * -------------------------------------------------------------------- */
        write(client_sock, buffer, strlen(buffer) + 1);
        close(client_sock);
    }
    
    return NULL;
}

/* ============================================================================
 * THREAD : SERVICE CONTENU DE FICHIER
 * ============================================================================
 * Port : PORT_CONTENU
 * Rôle : Lire et retourner le contenu d'un fichier
 * 
 * Protocole :
 *   Client → Serveur : nom du fichier (256 octets)
 *   Serveur → Client : contenu du fichier (max BUFFER_SIZE octets)
 * ============================================================================ */
void* thread_service_contenu(void* arg) {
    char log_msg[512];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /* --------------------------------------------------------------------
         * Attendre un client
         * -------------------------------------------------------------------- */
        int client_sock = accept(sockfd_contenu, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept contenu");
            }
            continue;
        }
        
        /* --------------------------------------------------------------------
         * Recevoir le nom du fichier
         * -------------------------------------------------------------------- */
        char nom_fichier[256];
        char buffer[BUFFER_SIZE];
        
        memset(nom_fichier, 0, sizeof(nom_fichier));
        read(client_sock, nom_fichier, sizeof(nom_fichier));
        
        snprintf(log_msg, sizeof(log_msg), "📄 [CONTENU] Demande pour: %s\n", nom_fichier);
        afficher_log(log_msg);
        
        /* --------------------------------------------------------------------
         * Lire le fichier
         * -------------------------------------------------------------------- */
        FILE *file = fopen(nom_fichier, "r");
        if (file == NULL) {
            sprintf(buffer, "Erreur: Impossible d'ouvrir le fichier");
            afficher_log("   ❌ Erreur d'ouverture\n");
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            
            // Lire jusqu'à BUFFER_SIZE-1 octets
            int n = fread(buffer, 1, BUFFER_SIZE - 1, file);
            buffer[n] = '\0';  // Terminer la chaîne
            fclose(file);
            
            snprintf(log_msg, sizeof(log_msg), "   ✅ Contenu envoyé (%d octets)\n", n);
            afficher_log(log_msg);
        }
        
        /* --------------------------------------------------------------------
         * Envoyer le contenu au client
         * -------------------------------------------------------------------- */
        write(client_sock, buffer, strlen(buffer) + 1);
        close(client_sock);
    }
    
    return NULL;
}

/* ============================================================================
 * THREAD : SERVICE DURÉE DE CONNEXION
 * ============================================================================
 * Port : PORT_DUREE
 * Rôle : Calculer la durée écoulée depuis la connexion du client
 * 
 * Protocole :
 *   Client → Serveur : timestamp de début (time_t)
 *   Serveur → Client : durée formatée "X minute(s) et Y seconde(s)"
 * ============================================================================ */
void* thread_service_duree(void* arg) {
    char log_msg[512];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /* --------------------------------------------------------------------
         * Attendre un client
         * -------------------------------------------------------------------- */
        int client_sock = accept(sockfd_duree, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept duree");
            }
            continue;
        }
        
        afficher_log("⏱️ [DURÉE] Demande de durée\n");
        
        /* --------------------------------------------------------------------
         * Recevoir le timestamp de début
         * -------------------------------------------------------------------- */
        time_t debut;
        read(client_sock, &debut, sizeof(time_t));
        
        /* --------------------------------------------------------------------
         * Calculer la durée écoulée
         * -------------------------------------------------------------------- */
        char buffer[BUFFER_SIZE];
        time_t maintenant;
        time(&maintenant);
        
        int duree_sec = (int)difftime(maintenant, debut);
        int minutes = duree_sec / 60;
        int secondes = duree_sec % 60;
        
        sprintf(buffer, "Durée de connexion: %d minute(s) et %d seconde(s)", 
                minutes, secondes);
        
        /* --------------------------------------------------------------------
         * Envoyer la durée au client
         * -------------------------------------------------------------------- */
        write(client_sock, buffer, strlen(buffer) + 1);
        snprintf(log_msg, sizeof(log_msg), "   ✅ Durée envoyée: %d:%02d\n", minutes, secondes);
        afficher_log(log_msg);
        
        close(client_sock);
    }
    
    return NULL;
}

/* ============================================================================
 * GESTIONNAIRE DE SIGNAL SIGINT (Ctrl+C)
 * ============================================================================
 * Rôle : Arrêter proprement le serveur
 * 
 * Pourquoi fermer les sockets ?
 *   - Les threads sont bloqués dans accept()
 *   - close() + shutdown() débloquent les accept() qui retournent une erreur
 *   - Les threads détectent serveur_actif=0 et se terminent
 * ============================================================================ */
void gestionnaire_sigint(int sig) {
    printf("\n\n🛑 Arrêt du serveur...\n");
    serveur_actif = 0;
    
    /* ------------------------------------------------------------------------
     * Fermer tous les sockets pour débloquer les accept()
     * shutdown(SHUT_RDWR) ferme la connexion dans les deux sens
     * close() libère le descripteur
     * ------------------------------------------------------------------------ */
    if (sockfd_auth >= 0) { 
        shutdown(sockfd_auth, SHUT_RDWR); 
        close(sockfd_auth); 
    }
    if (sockfd_date >= 0) { 
        shutdown(sockfd_date, SHUT_RDWR); 
        close(sockfd_date); 
    }
    if (sockfd_liste >= 0) { 
        shutdown(sockfd_liste, SHUT_RDWR); 
        close(sockfd_liste); 
    }
    if (sockfd_contenu >= 0) { 
        shutdown(sockfd_contenu, SHUT_RDWR); 
        close(sockfd_contenu); 
    }
    if (sockfd_duree >= 0) { 
        shutdown(sockfd_duree, SHUT_RDWR); 
        close(sockfd_duree); 
    }
}

/* ============================================================================
 * FONCTION PRINCIPALE
 * ============================================================================ */
int main(int argc, char *argv[]) {
    pthread_t threads[5];
    
    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS
     * ------------------------------------------------------------------------ */
    if (argc != 3) {
        printf("Usage: %s <username> <password>\n", argv[0]);
        printf("Exemple: %s admin admin123\n", argv[0]);
        return 1;
    }
    
    // Copier les identifiants dans les variables globales
    strcpy(global_username, argv[1]);
    strcpy(global_password, argv[2]);
    
    /* ------------------------------------------------------------------------
     * INSTALLATION DES GESTIONNAIRES DE SIGNAUX
     * ------------------------------------------------------------------------ */
    signal(SIGINT, gestionnaire_sigint);  // Ctrl+C
    signal(SIGPIPE, SIG_IGN);             // Ignorer les erreurs d'écriture
    
    /* ------------------------------------------------------------------------
     * AFFICHAGE DE BIENVENUE
     * ------------------------------------------------------------------------ */
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║   SERVEUR TCP MULTISERVICE DÉMARRÉ    ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("👤 Compte: %s / %s\n", global_username, global_password);
    printf("📋 Services disponibles:\n");
    printf("   🔐 Auth:     Port %d\n", PORT_AUTH);
    printf("   📅 Date:     Port %d\n", PORT_DATE);
    printf("   📂 Liste:    Port %d\n", PORT_LISTE);
    printf("   📄 Contenu:  Port %d\n", PORT_CONTENU);
    printf("   ⏱️  Durée:    Port %d\n", PORT_DUREE);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    /* ------------------------------------------------------------------------
     * CRÉATION DES SOCKETS SERVEURS
     * Un socket par service, chacun sur son port dédié
     * ------------------------------------------------------------------------ */
    printf("🔧 Création des sockets...\n");
    sockfd_auth = creer_socket_serveur(PORT_AUTH);
    sockfd_date = creer_socket_serveur(PORT_DATE);
    sockfd_liste = creer_socket_serveur(PORT_LISTE);
    sockfd_contenu = creer_socket_serveur(PORT_CONTENU);
    sockfd_duree = creer_socket_serveur(PORT_DUREE);
    
    // Vérifier que tous les sockets ont été créés avec succès
    if (sockfd_auth < 0 || sockfd_date < 0 || sockfd_liste < 0 || 
        sockfd_contenu < 0 || sockfd_duree < 0) {
        fprintf(stderr, "❌ Erreur: Impossible de créer les sockets\n");
        return 1;
    }
    
    printf("✅ Tous les sockets créés\n");
    printf("👂 En attente de clients...\n");
    printf("   (Ctrl+C pour arrêter)\n\n");
    
    /* ------------------------------------------------------------------------
     * LANCEMENT DES THREADS
     * Chaque thread gère un service et tourne en parallèle
     * ------------------------------------------------------------------------ */
    pthread_create(&threads[0], NULL, thread_service_auth, NULL);
    pthread_create(&threads[1], NULL, thread_service_date, NULL);
    pthread_create(&threads[2], NULL, thread_service_liste, NULL);
    pthread_create(&threads[3], NULL, thread_service_contenu, NULL);
    pthread_create(&threads[4], NULL, thread_service_duree, NULL);
    
    /* ------------------------------------------------------------------------
     * ATTENDRE LA FIN DE TOUS LES THREADS
     * pthread_join() bloque jusqu'à ce que le thread se termine
     * ------------------------------------------------------------------------ */
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("✅ Serveur arrêté proprement\n\n");
    
    return 0;
}

/* ============================================================================
 * EXPLICATION DE L'ARCHITECTURE MULTI-PORTS AVEC THREADS
 * ============================================================================
 * 
 * DIFFÉRENCES AVEC L'APPROCHE FORK (MULTI-PROCESSUS) :
 * 
 * 1. THREADS vs PROCESSUS :
 *    - Threads : partagent la même mémoire (variables globales communes)
 *    - Processus : mémoire séparée (chaque fils a sa propre copie)
 * 
 * 2. COMMUNICATION :
 *    - Threads : via variables globales (avec mutex pour la synchronisation)
 *    - Processus : via IPC (pipes, sockets, mémoire partagée, etc.)
 * 
 * 3. COÛT :
 *    - Threads : légers, création rapide, peu de mémoire
 *    - Processus : plus lourds, création plus lente, plus de mémoire
 * 
 * AVANTAGES DE CETTE ARCHITECTURE MULTI-PORTS :
 * 
 * - Séparation des services : chaque service est indépendant
 * - Scalabilité : facile d'ajouter de nouveaux services
 * - Robustesse : un service qui plante n'affecte pas les autres
 * - Simplicité du client : connexions courtes et ciblées
 * 
 * FONCTIONNEMENT :
 * 
 * 1. Au démarrage :
 *    - 5 sockets sont créés, chacun écoutant sur un port différent
 *    - 5 threads sont lancés, chacun gérant un service
 * 
 * 2. Pendant l'exécution :
 *    - Chaque thread attend des clients sur son port
 *    - Quand un client se connecte, le thread traite la requête
 *    - Une fois la réponse envoyée, la connexion est fermée
 *    - Le thread retourne à accept() pour le prochain client
 * 
 * 3. À l'arrêt (Ctrl+C) :
 *    - Le gestionnaire SIGINT met serveur_actif à 0
 *    - Tous les sockets sont fermés → accept() retourne une erreur
 *    - Chaque thread détecte serveur_actif=0 et se termine
 *    - Le main attend que tous les threads soient terminés
 * 
 * SYNCHRONISATION :
 * 
 * - log_mutex protège les affichages pour éviter le mélange
 * - serveur_actif est volatile pour que les threads voient les changements
 * - Pas besoin de mutex pour les sockets (chaque thread a le sien)
 * 
 * ============================================================================
 */

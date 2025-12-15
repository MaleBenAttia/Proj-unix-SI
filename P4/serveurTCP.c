/*
 * ============================================
 *  SERVEUR TCP MULTISERVICE MULTI-PORT
 * ============================================
 * 
 * Compilation:
 *   gcc serveurTCP.c -o serveurTCP -lpthread
 * 
 * Utilisation:
 *   ./serveurTCP [username] [password]
 *   Exemple: ./serveurTCP admin admin123
 */

#include "common.h"
#include <pthread.h>

// Variables globales
static char global_username[50];
static char global_password[50];
static volatile int serveur_actif = 1;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Sockets globaux
static int sockfd_auth;
static int sockfd_date;
static int sockfd_liste;
static int sockfd_contenu;
static int sockfd_duree;

/*
 * Affichage thread-safe
 */
void afficher_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    printf("%s", message);
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

/*
 * Créer un socket serveur
 */
int creer_socket_serveur(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/*
 * SERVICE AUTH: Authentification
 */
void* thread_service_auth(void* arg) {
    char log_msg[256];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(sockfd_auth, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept auth");
            }
            continue;
        }
        
        sprintf(log_msg, "🔐 [AUTH] Nouvelle demande d'authentification\n");
        afficher_log(log_msg);
        
        char username[50], password[50];
        
        memset(username, 0, sizeof(username));
        read(client_sock, username, sizeof(username));
        
        memset(password, 0, sizeof(password));
        read(client_sock, password, sizeof(password));
        
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
        
        write(client_sock, &auth_ok, sizeof(int));
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 1: Date et Heure
 */
void* thread_service_date(void* arg) {
    char log_msg[256];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
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
        
        char buffer[BUFFER_SIZE];
        time_t now;
        struct tm *timeinfo;
        
        time(&now);
        timeinfo = localtime(&now);
        strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", timeinfo);
        
        write(client_sock, buffer, strlen(buffer) + 1);
        afficher_log("   ✅ Date/Heure envoyée\n");
        
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 2: Liste des fichiers
 */
void* thread_service_liste(void* arg) {
    char log_msg[512];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(sockfd_liste, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept liste");
            }
            continue;
        }
        
        char chemin[256];
        char buffer[BUFFER_SIZE];
        
        memset(chemin, 0, sizeof(chemin));
        read(client_sock, chemin, sizeof(chemin));
        
        snprintf(log_msg, sizeof(log_msg), "📂 [LISTE] Demande pour: %s\n", chemin);
        afficher_log(log_msg);
        
        DIR *dir = opendir(chemin);
        if (dir == NULL) {
            sprintf(buffer, "Erreur: Impossible d'ouvrir le répertoire");
            afficher_log("   ❌ Erreur d'ouverture\n");
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                strcat(buffer, entry->d_name);
                strcat(buffer, "\n");
            }
            closedir(dir);
            afficher_log("   ✅ Liste envoyée\n");
        }
        
        write(client_sock, buffer, strlen(buffer) + 1);
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 3: Contenu fichier
 */
void* thread_service_contenu(void* arg) {
    char log_msg[512];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(sockfd_contenu, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept contenu");
            }
            continue;
        }
        
        char nom_fichier[256];
        char buffer[BUFFER_SIZE];
        
        memset(nom_fichier, 0, sizeof(nom_fichier));
        read(client_sock, nom_fichier, sizeof(nom_fichier));
        
        snprintf(log_msg, sizeof(log_msg), "📄 [CONTENU] Demande pour: %s\n", nom_fichier);
        afficher_log(log_msg);
        
        FILE *file = fopen(nom_fichier, "r");
        if (file == NULL) {
            sprintf(buffer, "Erreur: Impossible d'ouvrir le fichier");
            afficher_log("   ❌ Erreur d'ouverture\n");
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            int n = fread(buffer, 1, BUFFER_SIZE - 1, file);
            buffer[n] = '\0';
            fclose(file);
            snprintf(log_msg, sizeof(log_msg), "   ✅ Contenu envoyé (%d octets)\n", n);
            afficher_log(log_msg);
        }
        
        write(client_sock, buffer, strlen(buffer) + 1);
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 4: Durée connexion
 */
void* thread_service_duree(void* arg) {
    char log_msg[512];
    
    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(sockfd_duree, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (serveur_actif) {
                perror("accept duree");
            }
            continue;
        }
        
        afficher_log("⏱️ [DURÉE] Demande de durée\n");
        
        time_t debut;
        read(client_sock, &debut, sizeof(time_t));
        
        char buffer[BUFFER_SIZE];
        time_t maintenant;
        time(&maintenant);
        int duree_sec = (int)difftime(maintenant, debut);
        int minutes = duree_sec / 60;
        int secondes = duree_sec % 60;
        
        sprintf(buffer, "Durée de connexion: %d minute(s) et %d seconde(s)", 
                minutes, secondes);
        
        write(client_sock, buffer, strlen(buffer) + 1);
        snprintf(log_msg, sizeof(log_msg), "   ✅ Durée envoyée: %d:%02d\n", minutes, secondes);
        afficher_log(log_msg);
        
        close(client_sock);
    }
    
    return NULL;
}

/*
 * Gestionnaire de signal SIGINT (Ctrl+C)
 */
void gestionnaire_sigint(int sig) {
    printf("\n\n🛑 Arrêt du serveur...\n");
    serveur_actif = 0;
    
    // Fermer les sockets pour débloquer les accept()
    if (sockfd_auth >= 0) { shutdown(sockfd_auth, SHUT_RDWR); close(sockfd_auth); }
    if (sockfd_date >= 0) { shutdown(sockfd_date, SHUT_RDWR); close(sockfd_date); }
    if (sockfd_liste >= 0) { shutdown(sockfd_liste, SHUT_RDWR); close(sockfd_liste); }
    if (sockfd_contenu >= 0) { shutdown(sockfd_contenu, SHUT_RDWR); close(sockfd_contenu); }
    if (sockfd_duree >= 0) { shutdown(sockfd_duree, SHUT_RDWR); close(sockfd_duree); }
}

/*
 * MAIN
 */
int main(int argc, char *argv[]) {
    pthread_t threads[5];
    
    // Vérifier les arguments
    if (argc != 3) {
        printf("Usage: %s <username> <password>\n", argv[0]);
        printf("Exemple: %s admin admin123\n", argv[0]);
        return 1;
    }
    
    strcpy(global_username, argv[1]);
    strcpy(global_password, argv[2]);
    
    // Installer le gestionnaire de signal
    signal(SIGINT, gestionnaire_sigint);
    signal(SIGPIPE, SIG_IGN);
    
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
    
    // Créer les sockets
    printf("🔧 Création des sockets...\n");
    sockfd_auth = creer_socket_serveur(PORT_AUTH);
    sockfd_date = creer_socket_serveur(PORT_DATE);
    sockfd_liste = creer_socket_serveur(PORT_LISTE);
    sockfd_contenu = creer_socket_serveur(PORT_CONTENU);
    sockfd_duree = creer_socket_serveur(PORT_DUREE);
    
    if (sockfd_auth < 0 || sockfd_date < 0 || sockfd_liste < 0 || 
        sockfd_contenu < 0 || sockfd_duree < 0) {
        fprintf(stderr, "❌ Erreur: Impossible de créer les sockets\n");
        return 1;
    }
    
    printf("✅ Tous les sockets créés\n");
    printf("👂 En attente de clients...\n");
    printf("   (Ctrl+C pour arrêter)\n\n");
    
    // Lancer les threads
    pthread_create(&threads[0], NULL, thread_service_auth, NULL);
    pthread_create(&threads[1], NULL, thread_service_date, NULL);
    pthread_create(&threads[2], NULL, thread_service_liste, NULL);
    pthread_create(&threads[3], NULL, thread_service_contenu, NULL);
    pthread_create(&threads[4], NULL, thread_service_duree, NULL);
    
    // Attendre les threads
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("✅ Serveur arrêté proprement\n\n");
    
    return 0;
}

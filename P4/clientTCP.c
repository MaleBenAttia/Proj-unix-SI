/*
 * ============================================
 *   CLIENT TCP MULTISERVICE MULTI-PORT
 * ============================================
 * 
 * Compilation:
 *   gcc clientTCP.c -o clientTCP
 * 
 * Utilisation:
 *   ./clientTCP [serveur] [username] [password]
 *   Exemple: ./clientTCP localhost admin admin123
 */

#include "common.h"
#include <errno.h>

// Variables globales
static char serveur_nom[256];
static time_t debut_connexion;

/*
 * Connexion à un service
 */
int connecter_service(const char *serveur, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    // Timeout de 5 secondes
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    struct hostent *server = gethostbyname(serveur);
    if (server == NULL) {
        fprintf(stderr, "❌ Erreur: Serveur inconnu\n");
        close(sockfd);
        return -1;
    }
    
    struct sockaddr_in serveur_addr;
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(port);
    memcpy(&serveur_addr.sin_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr *)&serveur_addr, 
                sizeof(serveur_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/*
 * Authentification
 */
int authentifier(const char *serveur, const char *username, const char *password) {
    printf("🔐 Connexion au service d'authentification...\n");
    
    int sockfd = connecter_service(serveur, PORT_AUTH);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Impossible de se connecter au serveur\n");
        return 0;
    }
    
    printf("✅ Connecté au serveur!\n");
    printf("📤 Envoi des identifiants...\n");
    
    // Envoyer credentials
    write(sockfd, username, strlen(username) + 1);
    usleep(50000); // 50ms
    write(sockfd, password, strlen(password) + 1);
    
    // Recevoir résultat
    int resultat;
    ssize_t n = read(sockfd, &resultat, sizeof(int));
    close(sockfd);
    
    if (n != sizeof(int)) {
        fprintf(stderr, "❌ Erreur de réception du résultat\n");
        return 0;
    }
    
    if (resultat != AUTH_SUCCESS) {
        printf("❌ Échec de l'authentification!\n");
        return 0;
    }
    
    printf("✅ Authentification réussie!\n\n");
    return 1;
}

/*
 * SERVICE 1: Date et Heure
 */
void service_date() {
    printf("\n📅 [SERVICE DATE/HEURE]\n");
    printf("🔗 Connexion au service...\n");
    
    int sockfd = connecter_service(serveur_nom, PORT_DATE);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t n = read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    if (n <= 0) {
        fprintf(stderr, "❌ Erreur de réception\n");
        return;
    }
    
    printf("✅ Résultat: %s\n", buffer);
}

/*
 * SERVICE 2: Liste des fichiers
 */
void service_liste() {
    char chemin[256];
    
    printf("\n📂 [SERVICE LISTE FICHIERS]\n");
    printf("Entrez le chemin du répertoire (. pour courant): ");
    fflush(stdout);
    
    if (fgets(chemin, sizeof(chemin), stdin) == NULL) {
        return;
    }
    chemin[strcspn(chemin, "\n")] = 0; // Enlever le \n
    
    if (strlen(chemin) == 0) {
        strcpy(chemin, ".");
    }
    
    printf("🔗 Connexion au service...\n");
    
    int sockfd = connecter_service(serveur_nom, PORT_LISTE);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    write(sockfd, chemin, strlen(chemin) + 1);
    
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    printf("✅ Liste des fichiers dans '%s':\n", chemin);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("%s", buffer);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

/*
 * SERVICE 3: Contenu fichier
 */
void service_contenu() {
    char nom_fichier[256];
    
    printf("\n📄 [SERVICE CONTENU FICHIER]\n");
    printf("Entrez le nom du fichier: ");
    fflush(stdout);
    
    if (fgets(nom_fichier, sizeof(nom_fichier), stdin) == NULL) {
        return;
    }
    nom_fichier[strcspn(nom_fichier, "\n")] = 0; // Enlever le \n
    
    if (strlen(nom_fichier) == 0) {
        printf("❌ Nom de fichier vide\n");
        return;
    }
    
    printf("🔗 Connexion au service...\n");
    
    int sockfd = connecter_service(serveur_nom, PORT_CONTENU);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    write(sockfd, nom_fichier, strlen(nom_fichier) + 1);
    
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    printf("✅ Contenu du fichier '%s':\n", nom_fichier);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("%s", buffer);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

/*
 * SERVICE 4: Durée connexion
 */
void service_duree() {
    printf("\n⏱️  [SERVICE DURÉE CONNEXION]\n");
    printf("🔗 Connexion au service...\n");
    
    int sockfd = connecter_service(serveur_nom, PORT_DUREE);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    write(sockfd, &debut_connexion, sizeof(time_t));
    
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    printf("✅ Résultat: %s\n", buffer);
}

/*
 * Afficher le menu
 */
void afficher_menu() {
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║        SERVICES DISPONIBLES           ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("  [1] 📅 Date et Heure du serveur\n");
    printf("  [2] 📂 Liste des fichiers\n");
    printf("  [3] 📄 Contenu d'un fichier\n");
    printf("  [4] ⏱️  Durée de connexion\n");
    printf("  [0] 🚪 Quitter\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Votre choix: ");
    fflush(stdout);
}

/*
 * MAIN
 */
int main(int argc, char *argv[]) {
    char username[50], password[50];
    int choix;
    
    // Ignorer SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    // Vérifier les arguments
    if (argc == 4) {
        strcpy(serveur_nom, argv[1]);
        strcpy(username, argv[2]);
        strcpy(password, argv[3]);
    } else {
        // Demander les informations
        printf("\n");
        printf("╔═══════════════════════════════════════╗\n");
        printf("║     CLIENT TCP MULTISERVICE           ║\n");
        printf("╚═══════════════════════════════════════╝\n");
        
        printf("🖥️  Adresse du serveur (localhost): ");
        fflush(stdout);
        if (fgets(serveur_nom, sizeof(serveur_nom), stdin) == NULL) {
            return 1;
        }
        serveur_nom[strcspn(serveur_nom, "\n")] = 0;
        if (strlen(serveur_nom) == 0) {
            strcpy(serveur_nom, "localhost");
        }
        
        printf("👤 Username (admin): ");
        fflush(stdout);
        if (fgets(username, sizeof(username), stdin) == NULL) {
            return 1;
        }
        username[strcspn(username, "\n")] = 0;
        if (strlen(username) == 0) {
            strcpy(username, "admin");
        }
        
        printf("🔑 Password (admin123): ");
        fflush(stdout);
        if (fgets(password, sizeof(password), stdin) == NULL) {
            return 1;
        }
        password[strcspn(password, "\n")] = 0;
        if (strlen(password) == 0) {
            strcpy(password, "admin123");
        }
    }
    
    printf("\n");
    
    // Authentification
    if (!authentifier(serveur_nom, username, password)) {
        return 1;
    }
    
    // Enregistrer l'heure de début
    time(&debut_connexion);
    
    // Boucle principale
    while (1) {
        afficher_menu();
        
        if (scanf("%d", &choix) != 1) {
            while (getchar() != '\n'); // Vider le buffer
            printf("❌ Choix invalide\n");
            continue;
        }
        while (getchar() != '\n'); // Vider le buffer
        
        switch (choix) {
            case 0:
                printf("\n👋 Au revoir!\n\n");
                return 0;
                
            case 1:
                service_date();
                break;
                
            case 2:
                service_liste();
                break;
                
            case 3:
                service_contenu();
                break;
                
            case 4:
                service_duree();
                break;
                
            default:
                printf("❌ Choix invalide!\n");
        }
    }
    
    return 0;
}

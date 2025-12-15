/*
 * ============================================================================
 * Fichier : clientTCP.c
 * Description : Client TCP Multiservice Multi-Port
 * Le client se connecte à différents ports pour accéder à différents services
 * Compilation : gcc clientTCP.c -o clientTCP
 * Utilisation : ./clientTCP [serveur] [username] [password]
 * Exemple : ./clientTCP localhost admin admin123
 * Auteur : [Votre nom]
 * Date : Décembre 2025
 * ============================================================================
 */

#include "common.h"
#include <errno.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================
 * Ces variables sont accessibles dans toutes les fonctions du programme
 * ============================================================================ */

// Nom/adresse du serveur (ex: "localhost", "192.168.1.10")
static char serveur_nom[256];

// Heure de début de la session client (pour calculer la durée)
static time_t debut_connexion;

/* ============================================================================
 * FONCTION : connecter_service
 * ============================================================================
 * Rôle : Établir une connexion TCP à un service spécifique du serveur
 * 
 * Paramètres :
 *   - serveur : nom d'hôte ou adresse IP du serveur
 *   - port    : numéro de port du service (ex: 5000, 5001, etc.)
 * 
 * Retour :
 *   - Le descripteur du socket (>= 0) si succès
 *   - -1 en cas d'erreur
 * 
 * Étapes :
 *   1. Créer un socket TCP
 *   2. Configurer les timeouts (éviter de bloquer indéfiniment)
 *   3. Résoudre le nom d'hôte en adresse IP
 *   4. Se connecter au serveur
 * ============================================================================ */
int connecter_service(const char *serveur, int port) {
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 1 : Créer le socket
     * AF_INET = IPv4
     * SOCK_STREAM = TCP (connexion fiable, orientée flux)
     * 0 = protocole par défaut
     * ------------------------------------------------------------------------ */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 2 : Configurer les timeouts
     * Sans timeout, read() et write() peuvent bloquer indéfiniment si le
     * serveur ne répond pas. On fixe un timeout de 5 secondes.
     * ------------------------------------------------------------------------ */
    struct timeval timeout;
    timeout.tv_sec = 5;      // 5 secondes
    timeout.tv_usec = 0;     // 0 microsecondes
    
    // Timeout pour la réception (read)
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Timeout pour l'envoi (write)
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 3 : Résoudre le nom d'hôte
     * gethostbyname() convertit un nom (ex: "localhost") en adresse IP
     * Exemples :
     *   "localhost"    → 127.0.0.1
     *   "google.com"   → 142.250.185.46
     *   "192.168.1.10" → 192.168.1.10
     * ------------------------------------------------------------------------ */
    struct hostent *server = gethostbyname(serveur);
    if (server == NULL) {
        fprintf(stderr, "❌ Erreur: Serveur inconnu\n");
        close(sockfd);
        return -1;
    }
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 4 : Préparer l'adresse de destination
     * ------------------------------------------------------------------------ */
    struct sockaddr_in serveur_addr;
    memset(&serveur_addr, 0, sizeof(serveur_addr));       // Initialiser à zéro
    serveur_addr.sin_family = AF_INET;                    // IPv4
    serveur_addr.sin_port = htons(port);                  // Port (big-endian)
    
    // Copier l'adresse IP du serveur
    memcpy(&serveur_addr.sin_addr, server->h_addr, server->h_length);
    
    /* ------------------------------------------------------------------------
     * ÉTAPE 5 : Se connecter au serveur
     * connect() établit la connexion TCP (handshake à 3 voies)
     * ------------------------------------------------------------------------ */
    if (connect(sockfd, (struct sockaddr *)&serveur_addr, 
                sizeof(serveur_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    // Connexion établie avec succès !
    return sockfd;
}

/* ============================================================================
 * FONCTION : authentifier
 * ============================================================================
 * Rôle : S'authentifier auprès du serveur avant d'utiliser les services
 * 
 * Paramètres :
 *   - serveur  : nom d'hôte du serveur
 *   - username : nom d'utilisateur
 *   - password : mot de passe
 * 
 * Retour :
 *   - 1 si authentification réussie
 *   - 0 si échec
 * 
 * Protocole d'authentification :
 *   Client → Serveur : username (chaîne terminée par '\0')
 *   Client → Serveur : password (chaîne terminée par '\0')
 *   Serveur → Client : résultat (int) AUTH_SUCCESS ou AUTH_FAILURE
 * ============================================================================ */
int authentifier(const char *serveur, const char *username, const char *password) {
    printf("🔐 Connexion au service d'authentification...\n");
    
    /* ------------------------------------------------------------------------
     * Se connecter au service d'authentification (PORT_AUTH)
     * ------------------------------------------------------------------------ */
    int sockfd = connecter_service(serveur, PORT_AUTH);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Impossible de se connecter au serveur\n");
        return 0;
    }
    
    printf("✅ Connecté au serveur!\n");
    printf("📤 Envoi des identifiants...\n");
    
    /* ------------------------------------------------------------------------
     * Envoyer les identifiants
     * strlen() + 1 pour inclure le caractère '\0' de fin de chaîne
     * ------------------------------------------------------------------------ */
    write(sockfd, username, strlen(username) + 1);
    
    // Petit délai entre les deux envois (50ms)
    // Certains serveurs ont besoin d'un délai pour traiter les données
    usleep(50000); // 50000 microsecondes = 50 millisecondes
    
    write(sockfd, password, strlen(password) + 1);
    
    /* ------------------------------------------------------------------------
     * Recevoir le résultat de l'authentification
     * Le serveur envoie un entier : AUTH_SUCCESS ou AUTH_FAILURE
     * ------------------------------------------------------------------------ */
    int resultat;
    ssize_t n = read(sockfd, &resultat, sizeof(int));
    close(sockfd);  // Fermer la connexion (l'authentification est terminée)
    
    // Vérifier qu'on a bien reçu un entier complet
    if (n != sizeof(int)) {
        fprintf(stderr, "❌ Erreur de réception du résultat\n");
        return 0;
    }
    
    // Vérifier le résultat
    if (resultat != AUTH_SUCCESS) {
        printf("❌ Échec de l'authentification!\n");
        return 0;
    }
    
    printf("✅ Authentification réussie!\n\n");
    return 1;
}

/* ============================================================================
 * SERVICE 1 : Date et Heure
 * ============================================================================
 * Rôle : Demander la date et l'heure actuelles au serveur
 * Port : PORT_DATE
 * 
 * Protocole :
 *   Connexion → le serveur envoie immédiatement la date/heure → Déconnexion
 * ============================================================================ */
void service_date() {
    printf("\n📅 [SERVICE DATE/HEURE]\n");
    printf("🔗 Connexion au service...\n");
    
    /* ------------------------------------------------------------------------
     * Se connecter au service DATE
     * ------------------------------------------------------------------------ */
    int sockfd = connecter_service(serveur_nom, PORT_DATE);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    /* ------------------------------------------------------------------------
     * Recevoir la date/heure
     * Le serveur envoie automatiquement la date sans qu'on envoie de requête
     * ------------------------------------------------------------------------ */
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t n = read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);  // Fermer la connexion
    
    // Vérifier qu'on a bien reçu des données
    if (n <= 0) {
        fprintf(stderr, "❌ Erreur de réception\n");
        return;
    }
    
    /* ------------------------------------------------------------------------
     * Afficher le résultat
     * ------------------------------------------------------------------------ */
    printf("✅ Résultat: %s\n", buffer);
}

/* ============================================================================
 * SERVICE 2 : Liste des Fichiers
 * ============================================================================
 * Rôle : Obtenir la liste des fichiers d'un répertoire sur le serveur
 * Port : PORT_LISTE
 * 
 * Protocole :
 *   Client → Serveur : chemin du répertoire
 *   Serveur → Client : liste des fichiers (séparés par '\n')
 * ============================================================================ */
void service_liste() {
    char chemin[256];
    
    printf("\n📂 [SERVICE LISTE FICHIERS]\n");
    
    /* ------------------------------------------------------------------------
     * Demander le chemin à l'utilisateur
     * ------------------------------------------------------------------------ */
    printf("Entrez le chemin du répertoire (. pour courant): ");
    fflush(stdout);  // Forcer l'affichage immédiat
    
    if (fgets(chemin, sizeof(chemin), stdin) == NULL) {
        return;
    }
    
    // Enlever le caractère '\n' ajouté par fgets()
    chemin[strcspn(chemin, "\n")] = 0;
    
    // Si l'utilisateur n'a rien entré, utiliser "." (répertoire courant)
    if (strlen(chemin) == 0) {
        strcpy(chemin, ".");
    }
    
    printf("🔗 Connexion au service...\n");
    
    /* ------------------------------------------------------------------------
     * Se connecter au service LISTE
     * ------------------------------------------------------------------------ */
    int sockfd = connecter_service(serveur_nom, PORT_LISTE);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    /* ------------------------------------------------------------------------
     * Envoyer le chemin du répertoire
     * ------------------------------------------------------------------------ */
    write(sockfd, chemin, strlen(chemin) + 1);
    
    /* ------------------------------------------------------------------------
     * Recevoir la liste des fichiers
     * ------------------------------------------------------------------------ */
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    /* ------------------------------------------------------------------------
     * Afficher le résultat
     * ------------------------------------------------------------------------ */
    printf("✅ Liste des fichiers dans '%s':\n", chemin);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("%s", buffer);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

/* ============================================================================
 * SERVICE 3 : Contenu de Fichier
 * ============================================================================
 * Rôle : Lire le contenu d'un fichier sur le serveur
 * Port : PORT_CONTENU
 * 
 * Protocole :
 *   Client → Serveur : nom du fichier
 *   Serveur → Client : contenu du fichier (max BUFFER_SIZE octets)
 * ============================================================================ */
void service_contenu() {
    char nom_fichier[256];
    
    printf("\n📄 [SERVICE CONTENU FICHIER]\n");
    
    /* ------------------------------------------------------------------------
     * Demander le nom du fichier à l'utilisateur
     * ------------------------------------------------------------------------ */
    printf("Entrez le nom du fichier: ");
    fflush(stdout);
    
    if (fgets(nom_fichier, sizeof(nom_fichier), stdin) == NULL) {
        return;
    }
    
    // Enlever le '\n'
    nom_fichier[strcspn(nom_fichier, "\n")] = 0;
    
    // Vérifier que le nom n'est pas vide
    if (strlen(nom_fichier) == 0) {
        printf("❌ Nom de fichier vide\n");
        return;
    }
    
    printf("🔗 Connexion au service...\n");
    
    /* ------------------------------------------------------------------------
     * Se connecter au service CONTENU
     * ------------------------------------------------------------------------ */
    int sockfd = connecter_service(serveur_nom, PORT_CONTENU);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    /* ------------------------------------------------------------------------
     * Envoyer le nom du fichier
     * ------------------------------------------------------------------------ */
    write(sockfd, nom_fichier, strlen(nom_fichier) + 1);
    
    /* ------------------------------------------------------------------------
     * Recevoir le contenu du fichier
     * ------------------------------------------------------------------------ */
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    /* ------------------------------------------------------------------------
     * Afficher le résultat
     * ------------------------------------------------------------------------ */
    printf("✅ Contenu du fichier '%s':\n", nom_fichier);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("%s", buffer);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

/* ============================================================================
 * SERVICE 4 : Durée de Connexion
 * ============================================================================
 * Rôle : Calculer la durée écoulée depuis le début de la session
 * Port : PORT_DUREE
 * 
 * Protocole :
 *   Client → Serveur : timestamp de début (time_t)
 *   Serveur → Client : durée formatée "X minute(s) et Y seconde(s)"
 * 
 * Note : On envoie le timestamp de début au serveur, qui calcule la différence
 *        avec son heure actuelle. On pourrait aussi calculer côté client, mais
 *        cette approche permet de tester la communication de données binaires.
 * ============================================================================ */
void service_duree() {
    printf("\n⏱️  [SERVICE DURÉE CONNEXION]\n");
    printf("🔗 Connexion au service...\n");
    
    /* ------------------------------------------------------------------------
     * Se connecter au service DURÉE
     * ------------------------------------------------------------------------ */
    int sockfd = connecter_service(serveur_nom, PORT_DUREE);
    if (sockfd < 0) {
        fprintf(stderr, "❌ Erreur: Service indisponible\n");
        return;
    }
    
    /* ------------------------------------------------------------------------
     * Envoyer le timestamp de début de connexion
     * time_t est un type entier (généralement long int)
     * ------------------------------------------------------------------------ */
    write(sockfd, &debut_connexion, sizeof(time_t));
    
    /* ------------------------------------------------------------------------
     * Recevoir la durée formatée
     * ------------------------------------------------------------------------ */
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
    close(sockfd);
    
    /* ------------------------------------------------------------------------
     * Afficher le résultat
     * ------------------------------------------------------------------------ */
    printf("✅ Résultat: %s\n", buffer);
}

/* ============================================================================
 * FONCTION : afficher_menu
 * ============================================================================
 * Rôle : Afficher le menu principal avec les services disponibles
 * ============================================================================ */
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

/* ============================================================================
 * FONCTION PRINCIPALE
 * ============================================================================ */
int main(int argc, char *argv[]) {
    char username[50], password[50];
    int choix;
    
    /* ------------------------------------------------------------------------
     * IGNORER LE SIGNAL SIGPIPE
     * SIGPIPE est envoyé quand on écrit dans un socket fermé
     * Sans SIG_IGN, le programme se terminerait brutalement
     * Avec SIG_IGN, write() retourne -1 et on peut gérer l'erreur
     * ------------------------------------------------------------------------ */
    signal(SIGPIPE, SIG_IGN);
    
    /* ========================================================================
     * RÉCUPÉRATION DES PARAMÈTRES DE CONNEXION
     * ======================================================================== */
    
    /* ------------------------------------------------------------------------
     * CAS 1 : Les paramètres sont fournis en ligne de commande
     * Usage : ./clientTCP localhost admin admin123
     * ------------------------------------------------------------------------ */
    if (argc == 4) {
        strcpy(serveur_nom, argv[1]);
        strcpy(username, argv[2]);
        strcpy(password, argv[3]);
    } 
    /* ------------------------------------------------------------------------
     * CAS 2 : Demander les paramètres à l'utilisateur
     * ------------------------------------------------------------------------ */
    else {
        printf("\n");
        printf("╔═══════════════════════════════════════╗\n");
        printf("║     CLIENT TCP MULTISERVICE           ║\n");
        printf("╚═══════════════════════════════════════╝\n");
        
        /* --------------------------------------------------------------------
         * Adresse du serveur
         * -------------------------------------------------------------------- */
        printf("🖥️  Adresse du serveur (localhost): ");
        fflush(stdout);
        if (fgets(serveur_nom, sizeof(serveur_nom), stdin) == NULL) {
            return 1;
        }
        serveur_nom[strcspn(serveur_nom, "\n")] = 0;  // Enlever '\n'
        
        // Valeur par défaut si l'utilisateur n'entre rien
        if (strlen(serveur_nom) == 0) {
            strcpy(serveur_nom, "localhost");
        }
        
        /* --------------------------------------------------------------------
         * Nom d'utilisateur
         * -------------------------------------------------------------------- */
        printf("👤 Username (admin): ");
        fflush(stdout);
        if (fgets(username, sizeof(username), stdin) == NULL) {
            return 1;
        }
        username[strcspn(username, "\n")] = 0;
        
        if (strlen(username) == 0) {
            strcpy(username, "admin");
        }
        
        /* --------------------------------------------------------------------
         * Mot de passe
         * -------------------------------------------------------------------- */
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
    
    /* ========================================================================
     * AUTHENTIFICATION
     * ======================================================================== */
    
    if (!authentifier(serveur_nom, username, password)) {
        // Authentification échouée → quitter
        return 1;
    }
    
    /* ------------------------------------------------------------------------
     * Enregistrer l'heure de début de la session
     * Cette valeur sera utilisée par le service DURÉE
     * ------------------------------------------------------------------------ */
    time(&debut_connexion);
    
    /* ========================================================================
     * BOUCLE PRINCIPALE - MENU INTERACTIF
     * ======================================================================== */
    
    while (1) {
        /* --------------------------------------------------------------------
         * Afficher le menu et lire le choix
         * -------------------------------------------------------------------- */
        afficher_menu();
        
        // Lire un entier depuis stdin
        if (scanf("%d", &choix) != 1) {
            // Si scanf échoue (ex: l'utilisateur entre "abc")
            while (getchar() != '\n');  // Vider le buffer d'entrée
            printf("❌ Choix invalide\n");
            continue;  // Retourner au début de la boucle
        }
        
        // Vider le buffer (enlever le '\n' qui reste après scanf)
        while (getchar() != '\n');
        
        /* --------------------------------------------------------------------
         * Traiter le choix de l'utilisateur
         * -------------------------------------------------------------------- */
        switch (choix) {
            case 0:
                /* ============================================================
                 * QUITTER LE PROGRAMME
                 * ============================================================ */
                printf("\n👋 Au revoir!\n\n");
                return 0;
                
            case 1:
                /* ============================================================
                 * SERVICE DATE ET HEURE
                 * ============================================================ */
                service_date();
                break;
                
            case 2:
                /* ============================================================
                 * SERVICE LISTE DES FICHIERS
                 * ============================================================ */
                service_liste();
                break;
                
            case 3:
                /* ============================================================
                 * SERVICE CONTENU DE FICHIER
                 * ============================================================ */
                service_contenu();
                break;
                
            case 4:
                /* ============================================================
                 * SERVICE DURÉE DE CONNEXION
                 * ============================================================ */
                service_duree();
                break;
                
            default:
                /* ============================================================
                 * CHOIX INVALIDE
                 * ============================================================ */
                printf("❌ Choix invalide!\n");
        }
    }
    
    // Cette ligne n'est jamais atteinte (boucle infinie)
    // On sort par return 0 dans le case 0
    return 0;
}

/* ============================================================================
 * EXPLICATION DE L'ARCHITECTURE CLIENT MULTI-PORTS
 * ============================================================================
 * 
 * PRINCIPE GÉNÉRAL :
 * 
 * Ce client se connecte à un serveur offrant plusieurs services sur différents
 * ports. Contrairement à l'approche "connexion persistante", chaque service
 * nécessite une nouvelle connexion :
 * 
 * 1. Authentification (port 5000) → connexion, auth, déconnexion
 * 2. Pour chaque service :
 *    - Créer une nouvelle connexion sur le port du service
 *    - Envoyer la requête
 *    - Recevoir la réponse
 *    - Fermer la connexion immédiatement
 * 
 * AVANTAGES DE CETTE APPROCHE :
 * 
 * ✅ Simple et robuste
 *    - Pas de gestion d'état complexe
 *    - Chaque requête est indépendante
 * 
 * ✅ Tolérant aux pannes
 *    - Si un service plante, les autres continuent de fonctionner
 *    - Une connexion perdue n'affecte qu'une seule requête
 * 
 * ✅ Architecture microservices
 *    - Chaque service est totalement indépendant
 *    - Facile d'ajouter/retirer des services
 *    - Les services peuvent être sur des machines différentes
 * 
 * ✅ Pas de problème de synchronisation
 *    - Pas de gestion de session complexe
 *    - Pas de numéro de séquence à gérer
 * 
 * INCONVÉNIENTS :
 * 
 * ❌ Overhead de connexion
 *    - Chaque requête nécessite un handshake TCP (3 paquets)
 *    - Plus lent qu'une connexion persistante pour beaucoup de requêtes
 * 
 * ❌ Pas de contexte entre requêtes
 *    - Chaque requête est isolée
 *    - Pas de "conversation" entre le client et le serveur
 * 
 * FLUX D'EXÉCUTION TYPIQUE :
 * 
 * 1. Démarrage du client
 *    ├─ Récupérer les paramètres (serveur, username, password)
 *    └─ S'authentifier sur le port 5000
 * 
 * 2. Menu principal (boucle infinie)
 *    ├─ Afficher le menu
 *    ├─ Lire le choix de l'utilisateur
 *    └─ Exécuter le service choisi :
 *       ├─ Connexion au port du service
 *       ├─ Envoi de la requête
 *       ├─ Réception de la réponse
 *       └─ Déconnexion
 * 
 * 3. Quitter (choix 0)
 *    └─ Terminer proprement le programme
 * 
 * GESTION DES ERREURS :
 * 
 * - Timeouts sur les sockets (5 secondes)
 *   → Évite de bloquer indéfiniment si le serveur ne répond pas
 * 
 * - Signal SIGPIPE ignoré
 *   → Évite la terminaison brutale si on écrit sur un socket fermé
 * 
 * - Vérification systématique des retours de fonctions
 *   → read(), write(), connect() peuvent échouer
 * 
 * DONNÉES PARTAGÉES :
 * 
 * - serveur_nom : utilisé par toutes les fonctions de service
 * - debut_connexion : enregistré au début, utilisé par service_duree()
 * 
 * COMPARAISON AVEC L'APPROCHE "CONNEXION PERSISTANTE" :
 * 
 * Connexion persistante (Document 2) :
 *   Client → Serveur : connect()
 *   Client ↔ Serveur : authentification
 *   Client ↔ Serveur : service1
 *   Client ↔ Serveur : service2
 *   Client ↔ Serveur : service3
 *   Client → Serveur : déconnexion
 * 
 * Multi-ports (ce document) :
 *   Client → Serveur (port 5000) : connect() → auth → close()
 *   Client → Serveur (port 5001) : connect() → service1 → close()
 *   Client → Serveur (port 5002) : connect() → service2 → close()
 *   Client → Serveur (port 5003) : connect() → service3 → close()
 * 
 * QUAND UTILISER CETTE APPROCHE ?
 * 
 * ✅ Services indépendants et rapides (< 1 seconde)
 * ✅ Architecture distribuée (services sur différentes machines)
 * ✅ Besoin de haute disponibilité (isoler les pannes)
 * ✅ Pas besoin de maintenir un contexte entre requêtes
 * 
 * ❌ Ne PAS utiliser pour :
 *    - Streaming de données (vidéo, audio)
 *    - Chat en temps réel
 *    - Jeux multi-joueurs
 *    - Sessions avec état complexe
 * 
 * ============================================================================
 */

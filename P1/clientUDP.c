/*
 * ============================================================================
 * Fichier : clientUDP.c
 * Description : Client UDP - Envoie un nombre n et reçoit n nombres aléatoires
 * Utilisation : ./clientUDP <nom_serveur> <port>
 * Exemple : ./clientUDP localhost 5000
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
    
    int sockfd;                          // Descripteur du socket UDP
    struct sockaddr_in serveur_addr;     // Structure contenant l'adresse du serveur
    struct hostent *serveur;             // Structure contenant les infos du serveur
    int port;                            // Numéro de port du serveur
    int n;                               // Nombre aléatoire à envoyer au serveur
    int buffer[BUFFER_SIZE];             // Buffer pour recevoir les nombres
    socklen_t len;                       // Taille de la structure d'adresse

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS
     * ------------------------------------------------------------------------ */
    
    // Le programme attend 3 arguments : nom_programme, nom_serveur, port
    if (argc != 3) {
        printf("Usage: %s <nom_serveur> <port>\n", argv[0]);
        exit(1);  // Code de sortie 1 = erreur
    }

    // Convertir le port (chaîne) en entier
    port = atoi(argv[2]);

    /* ------------------------------------------------------------------------
     * CRÉATION DU SOCKET UDP
     * ------------------------------------------------------------------------ */
    
    // socket(domaine, type, protocole)
    // - AF_INET : Famille IPv4
    // - SOCK_DGRAM : Type UDP (datagramme)
    // - 0 : Protocole par défaut (UDP pour SOCK_DGRAM)
    // Retourne : descripteur de socket (entier >= 0) ou -1 si erreur
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erreur socket");  // Affiche le message d'erreur système
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * RÉSOLUTION DU NOM D'HÔTE
     * ------------------------------------------------------------------------ */
    
    // Convertir le nom d'hôte (ex: "localhost") en adresse IP
    // Retourne un pointeur vers struct hostent ou NULL si échec
    serveur = gethostbyname(argv[1]);
    if (serveur == NULL) {
        printf("Erreur: serveur inconnu\n");
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * CONFIGURATION DE L'ADRESSE DU SERVEUR
     * ------------------------------------------------------------------------ */
    
    // Initialiser toute la structure à zéro
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    
    // sin_family : Famille d'adresses (AF_INET = IPv4)
    serveur_addr.sin_family = AF_INET;
    
    // sin_port : Numéro de port en network byte order (big-endian)
    // htons() = Host TO Network Short (conversion little-endian -> big-endian)
    serveur_addr.sin_port = htons(port);
    
    // sin_addr : Adresse IP du serveur
    // Copier l'adresse depuis la structure hostent vers sockaddr_in
    memcpy(&serveur_addr.sin_addr, serveur->h_addr, serveur->h_length);

    /* ------------------------------------------------------------------------
     * GÉNÉRATION D'UN NOMBRE ALÉATOIRE
     * ------------------------------------------------------------------------ */
    
    // Initialiser le générateur de nombres aléatoires avec l'heure actuelle
    // time(NULL) retourne le nombre de secondes depuis le 1er janvier 1970
    srand(time(NULL));
    
    // Générer un nombre entre 1 et NMAX (inclus)
    // rand() % NMAX donne 0 à (NMAX-1), on ajoute 1 pour avoir 1 à NMAX
    n = (rand() % NMAX) + 1;
    
    printf("Nombre genere: %d\n", n);

    /* ------------------------------------------------------------------------
     * ENVOI DU NOMBRE AU SERVEUR
     * ------------------------------------------------------------------------ */
    
    // sendto(socket, données, taille, flags, adresse_dest, taille_adresse)
    // - sockfd : notre socket UDP
    // - &n : adresse du nombre à envoyer
    // - sizeof(int) : taille des données (4 octets)
    // - 0 : pas d'options spéciales
    // - (struct sockaddr *)&serveur_addr : adresse du serveur (castée)
    // - len : taille de la structure d'adresse
    len = sizeof(serveur_addr);
    sendto(sockfd, &n, sizeof(int), 0, (struct sockaddr *)&serveur_addr, len);
    printf("Nombre envoye au serveur\n");

    /* ------------------------------------------------------------------------
     * RÉCEPTION DE LA RÉPONSE
     * ------------------------------------------------------------------------ */
    
    // recvfrom(socket, buffer, taille_max, flags, adresse_source, taille_adresse)
    // Cette fonction BLOQUE jusqu'à la réception de données
    // Retourne : nombre d'octets reçus ou -1 si erreur
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serveur_addr, &len);

    /* ------------------------------------------------------------------------
     * AFFICHAGE DES RÉSULTATS
     * ------------------------------------------------------------------------ */
    
    printf("Nombres recus du serveur: ");
    
    // Parcourir et afficher les n nombres reçus
    for (int i = 0; i < n; i++) {
        printf("%d ", buffer[i]);
    }
    printf("\n");

    /* ------------------------------------------------------------------------
     * FERMETURE DU SOCKET
     * ------------------------------------------------------------------------ */
    
    // Libérer les ressources système associées au socket
    close(sockfd);
    
    return 0;  // Code de sortie 0 = succès
}

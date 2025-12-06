/*
 * ============================================================================
 * Fichier : serveurUDP.c
 * Description : Serveur UDP - Reçoit un nombre n et envoie n nombres aléatoires
 * Utilisation : ./serveurUDP <port>
 * Exemple : ./serveurUDP 5000
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
    
    int sockfd;                          // Descripteur du socket UDP
    struct sockaddr_in serveur_addr;     // Adresse du serveur (nous)
    struct sockaddr_in client_addr;      // Adresse du client (remplie par recvfrom)
    int port;                            // Port d'écoute du serveur
    int n;                               // Nombre reçu du client
    int buffer[BUFFER_SIZE];             // Tableau pour stocker les nombres à envoyer
    socklen_t len;                       // Taille de la structure d'adresse

    /* ------------------------------------------------------------------------
     * VÉRIFICATION DES ARGUMENTS
     * ------------------------------------------------------------------------ */
    
    // Le programme attend 2 arguments : nom_programme et port
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Convertir le port de chaîne en entier
    port = atoi(argv[1]);

    /* ------------------------------------------------------------------------
     * CRÉATION DU SOCKET UDP
     * ------------------------------------------------------------------------ */
    
    // Créer un socket UDP (même appel que le client)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erreur socket");
        exit(1);
    }

    /* ------------------------------------------------------------------------
     * CONFIGURATION DE L'ADRESSE LOCALE
     * ------------------------------------------------------------------------ */
    
    // Initialiser la structure à zéro
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    
    // sin_family : IPv4
    serveur_addr.sin_family = AF_INET;
    
    // sin_addr : INADDR_ANY = écouter sur TOUTES les interfaces réseau
    // Valeur 0 = 0.0.0.0 (accepte connexions de n'importe quelle interface)
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    
    // sin_port : Port d'écoute en network byte order
    serveur_addr.sin_port = htons(port);

    /* ------------------------------------------------------------------------
     * LIAISON DU SOCKET AU PORT (BIND)
     * ------------------------------------------------------------------------ */
    
    // bind(socket, adresse, taille_adresse)
    // Attache le socket à l'adresse et au port spécifiés
    // OBLIGATOIRE côté serveur pour "réserver" le port
    // Retourne : 0 si succès, -1 si erreur
    if (bind(sockfd, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr)) < 0) {
        perror("Erreur bind");
        exit(1);
    }

    printf("Serveur demarre sur le port %d\n", port);
    printf("En attente de requetes...\n\n");

    /* ------------------------------------------------------------------------
     * INITIALISATION DU GÉNÉRATEUR ALÉATOIRE
     * ------------------------------------------------------------------------ */
    
    // Initialiser avec l'heure actuelle pour avoir des nombres différents
    srand(time(NULL));

    /* ------------------------------------------------------------------------
     * BOUCLE PRINCIPALE - SERVEUR INFINI
     * ------------------------------------------------------------------------ */
    
    // Boucle infinie : le serveur ne s'arrête jamais (Ctrl+C pour arrêter)
    while (1) {
        
        /* --------------------------------------------------------------------
         * RÉCEPTION DU NOMBRE N DU CLIENT
         * -------------------------------------------------------------------- */
        
        // recvfrom() BLOQUE ici en attendant qu'un client envoie des données
        // Elle remplit automatiquement client_addr avec l'adresse du client
        len = sizeof(client_addr);
        recvfrom(sockfd, &n, sizeof(int), 0, (struct sockaddr *)&client_addr, &len);
        
        printf("Nombre recu: %d\n", n);

        /* --------------------------------------------------------------------
         * GÉNÉRATION DE N NOMBRES ALÉATOIRES
         * -------------------------------------------------------------------- */
        
        printf("Generation de %d nombres: ", n);
        
        // Boucle pour générer n nombres aléatoires
        for (int i = 0; i < n; i++) {
            // Générer un nombre entre 1 et MAX_RANDOM
            buffer[i] = (rand() % MAX_RANDOM) + 1;
            printf("%d ", buffer[i]);
        }
        printf("\n");

        /* --------------------------------------------------------------------
         * ENVOI DES NOMBRES AU CLIENT
         * -------------------------------------------------------------------- */
        
        // sendto() envoie les données à l'adresse client_addr
        // Cette adresse a été automatiquement remplie par recvfrom()
        // On envoie n entiers, soit n * sizeof(int) octets
        sendto(sockfd, buffer, n * sizeof(int), 0, 
               (struct sockaddr *)&client_addr, len);
        
        printf("Nombres envoyes au client\n\n");
        
        // Retour au début de la boucle pour attendre le prochain client
    }

    /* ------------------------------------------------------------------------
     * FERMETURE (code jamais atteint car boucle infinie)
     * ------------------------------------------------------------------------ */
    
    close(sockfd);
    return 0;
}

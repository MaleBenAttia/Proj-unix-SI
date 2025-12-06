/*
 * ============================================================================
 * Fichier : common.h
 * Description : Fichier d'en-tête commun pour client et serveur UDP
 * Auteur : [Votre Nom]
 * Date : Décembre 2025
 * ============================================================================
 */

#ifndef COMMON_H
#define COMMON_H

/* ============================================================================
 * BIBLIOTHÈQUES SYSTÈME
 * ============================================================================ */

#include <stdio.h>       // Entrées/sorties standard (printf, scanf, perror)
#include <stdlib.h>      // Fonctions utilitaires (exit, atoi, rand, srand)
#include <string.h>      // Manipulation de chaînes (strlen, memset, memcpy)
#include <unistd.h>      // Appels système POSIX (close, read, write)
#include <arpa/inet.h>   // Fonctions réseau (htons, inet_ntop)
#include <sys/socket.h>  // Interface socket (socket, bind, sendto, recvfrom)
#include <netdb.h>       // Résolution noms d'hôte (gethostbyname)
#include <time.h>        // Manipulation du temps (time, srand)

/* ============================================================================
 * CONSTANTES DU PROJET
 * ============================================================================ */

// Nombre aléatoire maximum que le client peut générer
#define NMAX 10

// Taille du buffer pour la communication (1024 octets = 1 Ko)
#define BUFFER_SIZE 1024

// Valeur maximale des nombres aléatoires générés par le serveur
#define MAX_RANDOM 100

#endif // COMMON_H

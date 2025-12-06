/*
 * ============================================================================
 * Fichier : common.h
 * Description : Fichier d'en-tête commun pour client et serveur TCP Phase 1
 * Phase : Monoclient/Monoserveur avec un service (Date/Heure)
 * Auteur : [Votre Nom]
 * Date : Décembre 2025
 * ============================================================================
 */

#ifndef COMMON_H
#define COMMON_H

/* ============================================================================
 * BIBLIOTHÈQUES SYSTÈME
 * ============================================================================ */

#include <stdio.h>       // printf, scanf, perror
#include <stdlib.h>      // exit, atoi
#include <string.h>      // strlen, strcmp, memset
#include <unistd.h>      // close, read, write
#include <arpa/inet.h>   // htons
#include <sys/socket.h>  // socket, bind, listen, accept, connect
#include <netdb.h>       // gethostbyname
#include <time.h>        // time, localtime, strftime

/* ============================================================================
 * CONSTANTES DU PROJET
 * ============================================================================ */

// Taille du buffer de communication (1024 octets)
#define BUFFER_SIZE 1024

// Port par défaut (peut être changé en argument)
#define PORT_DEFAULT 6000

/* ============================================================================
 * CODES DES SERVICES
 * ============================================================================ */

// Code pour demander la date et l'heure
#define SERVICE_DATE 1

// Code pour terminer la connexion
#define SERVICE_FIN 0

#endif // COMMON_H

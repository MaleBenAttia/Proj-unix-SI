/*
 * ============================================
 *     COMMON.H - TCP MULTISERVICE
 * ============================================
 * Fichier d'en-tête commun pour client/serveur TCP
 */

#ifndef COMMON_H
#define COMMON_H

// ===== BIBLIOTHÈQUES SYSTÈME =====
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

// ===== BIBLIOTHÈQUES RÉSEAU =====
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

// ===== CONSTANTES =====
#define BUFFER_SIZE 4096        // Taille du buffer
#define PORT_DEFAULT 6000       // Port par défaut

// ===== CODES DES SERVICES =====
#define SERVICE_DATE 1                  // Service Date et Heure
#define SERVICE_LISTE_FICHIERS 2        // Service Liste fichiers
#define SERVICE_CONTENU_FICHIER 3       // Service Contenu fichier
#define SERVICE_DUREE_CONNEXION 4       // Service Durée connexion
#define SERVICE_FIN 0                   // Quitter

#endif

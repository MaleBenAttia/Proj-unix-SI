/*
 * ============================================
 *     COMMON.H - TCP MULTISERVICE MULTI-PORT
 * ============================================
 * Fichier d'en-tête commun pour client/serveur TCP
 * Chaque service écoute sur un port différent
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

// ===== PORTS DES SERVICES =====
#define PORT_AUTH 6000          // Port pour l'authentification
#define PORT_DATE 6001          // Port pour Date et Heure
#define PORT_LISTE 6002         // Port pour Liste fichiers
#define PORT_CONTENU 6003       // Port pour Contenu fichier
#define PORT_DUREE 6004         // Port pour Durée connexion

// ===== CODES DES SERVICES =====
#define SERVICE_DATE 1
#define SERVICE_LISTE_FICHIERS 2
#define SERVICE_CONTENU_FICHIER 3
#define SERVICE_DUREE_CONNEXION 4
#define SERVICE_FIN 0

// ===== CODES DE RÉPONSE =====
#define AUTH_SUCCESS 1
#define AUTH_FAILURE 0
#define SERVICE_OK 1
#define SERVICE_ERROR 0

#endif

/*
 * ============================================================================
 * Fichier : common.h
 * Description : Fichier d'en-tête commun TCP Phase 3
 * Phase : Multiclient/Multiservice (4 services disponibles)
 * Auteur : [Votre Nom]
 * Date : Décembre 2025
 * ============================================================================
 */

#ifndef COMMON_H
#define COMMON_H

/* ============================================================================
 * BIBLIOTHÈQUES SYSTÈME
 * ============================================================================ */

#include <stdio.h>       // printf, scanf, perror, fopen, fread, fclose
#include <stdlib.h>      // exit, atoi
#include <string.h>      // strlen, strcmp, memset, strcat, strcspn
#include <unistd.h>      // close, read, write, fork, getpid
#include <arpa/inet.h>   // htons
#include <sys/socket.h>  // socket, bind, listen, accept, connect
#include <netdb.h>       // gethostbyname
#include <time.h>        // time, localtime, strftime, difftime
#include <sys/wait.h>    // waitpid, WNOHANG
#include <signal.h>      // signal, SIGCHLD
#include <dirent.h>      // opendir, readdir, closedir

/* ============================================================================
 * CONSTANTES DU PROJET
 * ============================================================================ */

// Buffer plus grand pour les fichiers volumineux
#define BUFFER_SIZE 4096

#define PORT_DEFAULT 6000

/* ============================================================================
 * CODES DES SERVICES DISPONIBLES
 * ============================================================================ */

#define SERVICE_DATE 1                // Date et heure système
#define SERVICE_LISTE_FICHIERS 2      // Liste des fichiers d'un répertoire
#define SERVICE_CONTENU_FICHIER 3     // Contenu d'un fichier
#define SERVICE_DUREE_CONNEXION 4     // Durée écoulée depuis la connexion
#define SERVICE_FIN 0                 // Déconnexion

#endif // COMMON_H

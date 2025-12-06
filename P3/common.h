/*
 * ============================================================================
 * Fichier : common.h
 * Description : Fichier d'en-tête commun TCP Phase 2
 * Phase : Multiclient/Monoserveur avec fork()
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
#include <unistd.h>      // close, read, write, fork, getpid
#include <arpa/inet.h>   // htons
#include <sys/socket.h>  // socket, bind, listen, accept, connect
#include <netdb.h>       // gethostbyname
#include <time.h>        // time, localtime, strftime
#include <sys/wait.h>    // waitpid, WNOHANG
#include <signal.h>      // signal, SIGCHLD

/* ============================================================================
 * CONSTANTES DU PROJET
 * ============================================================================ */

#define BUFFER_SIZE 1024
#define PORT_DEFAULT 6000

/* ============================================================================
 * CODES DES SERVICES
 * ============================================================================ */

#define SERVICE_DATE 1
#define SERVICE_FIN 0

#endif // COMMON_H

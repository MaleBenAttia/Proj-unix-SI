##############################################################################
# Fichier : Makefile
# Description : Script de compilation automatique pour Client/Serveur UDP
# Auteur : [Votre Nom]
# Date : Décembre 2025
##############################################################################

# ============================================================================
# VARIABLES DE CONFIGURATION
# ============================================================================

# Compilateur C à utiliser
CC = gcc

# Options de compilation
# -Wall : Active tous les avertissements (warnings)
CFLAGS = -Wall

# ============================================================================
# RÈGLES DE COMPILATION
# ============================================================================

# Règle par défaut (exécutée si on tape juste "make")
# Compile le client ET le serveur
all: clientUDP serveurUDP

# ----------------------------------------------------------------------------
# Règle pour compiler le CLIENT UDP
# ----------------------------------------------------------------------------
# Syntaxe : cible: dépendances
#           commande
# 
# clientUDP dépend de clientUDP.c et common.h
# Si l'un de ces fichiers est modifié, clientUDP sera recompilé
clientUDP: clientUDP.c common.h
	$(CC) $(CFLAGS) -o clientUDP clientUDP.c
	# $(CC) sera remplacé par "gcc"
	# $(CFLAGS) sera remplacé par "-Wall"
	# Commande finale : gcc -Wall -o clientUDP clientUDP.c

# ----------------------------------------------------------------------------
# Règle pour compiler le SERVEUR UDP
# ----------------------------------------------------------------------------
serveurUDP: serveurUDP.c common.h
	$(CC) $(CFLAGS) -o serveurUDP serveurUDP.c
	# Commande finale : gcc -Wall -o serveurUDP serveurUDP.c

# ============================================================================
# RÈGLES DE NETTOYAGE
# ============================================================================

# ----------------------------------------------------------------------------
# Règle CLEAN : Supprime les exécutables et fichiers objets
# ----------------------------------------------------------------------------
# Usage : make clean
# Supprime tous les fichiers générés pour repartir à zéro
clean:
	rm -f clientUDP serveurUDP *.o
	# rm -f : supprime les fichiers (force, pas d'erreur si inexistant)
	# *.o : tous les fichiers objets (s'il y en a)

# ============================================================================
# DÉCLARATION DES CIBLES PHONY
# ============================================================================

# Les cibles "phony" ne correspondent pas à des fichiers réels
# Cela évite les conflits si un fichier s'appelle "all" ou "clean"
.PHONY: all clean

##############################################################################
# Fichier : Makefile
# Description : Compilation automatique Client/Serveur TCP Phase 3
# Phase : Multiclient/Multiservice
# Auteur : Malek Ben Attia et salim Younes
# Date : Décembre 2025
##############################################################################

# Compilateur C
CC = gcc

# Options de compilation
# -Wall : Affiche tous les avertissements
CFLAGS = -Wall

# ============================================================================
# RÈGLES DE COMPILATION
# ============================================================================

# Règle par défaut : compiler client et serveur
all: clientTCP serveurTCP

# Compiler le client TCP Phase 3
# Dépend de clientTCP.c et common.h
# Si l'un de ces fichiers change, le client est recompilé
clientTCP: clientTCP.c common.h
	$(CC) $(CFLAGS) -o clientTCP clientTCP.c

# Compiler le serveur TCP Phase 3
# Dépend de serveurTCP.c et common.h
serveurTCP: serveurTCP.c common.h
	$(CC) $(CFLAGS) -o serveurTCP serveurTCP.c

# Supprimer les exécutables et fichiers objets
clean:
	rm -f clientTCP serveurTCP *.o

# Déclaration des cibles phony (ne correspondent pas à des fichiers)
.PHONY: all clean

##############################################################################
# Fichier : Makefile
# Description : Compilation automatique Client/Serveur TCP Phase 1
# Auteur : [Votre Nom]
# Date : Décembre 2025
##############################################################################

# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall

# ============================================================================
# RÈGLES DE COMPILATION
# ============================================================================

# Règle par défaut : compile client et serveur
all: clientTCP serveurTCP

# Compiler le client TCP
clientTCP: clientTCP.c common.h
	$(CC) $(CFLAGS) -o clientTCP clientTCP.c

# Compiler le serveur TCP
serveurTCP: serveurTCP.c common.h
	$(CC) $(CFLAGS) -o serveurTCP serveurTCP.c

# Supprimer les exécutables
clean:
	rm -f clientTCP serveurTCP *.o

# Déclaration des cibles phony
.PHONY: all clean

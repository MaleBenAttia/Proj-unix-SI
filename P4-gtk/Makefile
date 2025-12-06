# ============================================
#     MAKEFILE - TCP MULTISERVICE GTK
# ============================================
# 
# Utilisation:
#   make          - Compile tout
#   make client   - Compile le client
#   make serveur  - Compile le serveur
#   make clean    - Nettoie
#   make help     - Affiche l'aide

# ===== CONFIGURATION =====
CC = gcc
CFLAGS = -Wall `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

# ===== CIBLES PRINCIPALES =====

# Compiler tout
all: clientTCP_gtk serveurTCP_gtk
	@echo ""
	@echo "╔════════════════════════════════════════╗"
	@echo "║   ✅ COMPILATION RÉUSSIE !             ║"
	@echo "╚════════════════════════════════════════╝"
	@echo ""
	@echo "📋 Fichiers créés:"
	@echo "   - clientTCP_gtk"
	@echo "   - serveurTCP_gtk"
	@echo ""
	@echo "🚀 Pour tester:"
	@echo "   Terminal 1: ./serveurTCP_gtk"
	@echo "   Terminal 2: ./clientTCP_gtk"
	@echo ""

# Compiler le client
client: clientTCP_gtk
	@echo "✅ Client TCP GTK compilé!"

# Compiler le serveur
serveur: serveurTCP_gtk
	@echo "✅ Serveur TCP GTK compilé!"

# ===== RÈGLES DE COMPILATION =====

clientTCP_gtk: clientTCP_gtk.c common.h
	@echo "🔨 Compilation du client TCP multiservice..."
	$(CC) $(CFLAGS) clientTCP_gtk.c -o clientTCP_gtk $(LIBS)

serveurTCP_gtk: serveurTCP_gtk.c common.h
	@echo "🔨 Compilation du serveur TCP multiservice..."
	$(CC) $(CFLAGS) serveurTCP_gtk.c -o serveurTCP_gtk $(LIBS)

# ===== NETTOYAGE =====

clean:
	@echo "🧹 Nettoyage des fichiers compilés..."
	rm -f clientTCP_gtk serveurTCP_gtk
	@echo "✅ Nettoyage terminé!"

# ===== AIDE =====

help:
	@echo ""
	@echo "╔════════════════════════════════════════╗"
	@echo "║   MAKEFILE TCP MULTISERVICE GTK        ║"
	@echo "╚════════════════════════════════════════╝"
	@echo ""
	@echo "📋 Commandes disponibles:"
	@echo ""
	@echo "  make          - Compile tout (client + serveur)"
	@echo "  make client   - Compile uniquement le client"
	@echo "  make serveur  - Compile uniquement le serveur"
	@echo "  make clean    - Supprime les fichiers compilés"
	@echo "  make help     - Affiche cette aide"
	@echo ""
	@echo "📁 Fichiers nécessaires:"
	@echo "  - common.h            (en-têtes)"
	@echo "  - clientTCP_gtk.c     (code client)"
	@echo "  - serveurTCP_gtk.c    (code serveur)"
	@echo ""
	@echo "🔧 Prérequis:"
	@echo "  - gcc (compilateur)"
	@echo "  - GTK+ 3.0 (libgtk-3-dev)"
	@echo ""
	@echo "📦 Installation GTK:"
	@echo "  Ubuntu/Debian:"
	@echo "    sudo apt-get install libgtk-3-dev"
	@echo ""
	@echo "  Fedora:"
	@echo "    sudo dnf install gtk3-devel"
	@echo ""
	@echo "🎯 Services disponibles:"
	@echo "  [1] Date et Heure du serveur"
	@echo "  [2] Liste des fichiers d'un répertoire"
	@echo "  [3] Contenu d'un fichier"
	@echo "  [4] Durée de connexion"
	@echo ""

.PHONY: all client serveur clean help

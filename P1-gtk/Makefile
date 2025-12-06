# Makefile pour compiler les programmes client/serveur UDP et TCP avec GTK
# Utilisation:
#   make all      - Compile tous les programmes
#   make clean    - Supprime les fichiers compilés
#   make udp      - Compile uniquement UDP
#   make tcp      - Compile uniquement TCP

# Variables
CC = gcc
CFLAGS = -Wall `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

# Cibles
all: clientUDP_gtk serveurUDP_gtk clientTCP_gtk serveurTCP_gtk

udp: clientUDP_gtk serveurUDP_gtk

tcp: clientTCP_gtk serveurTCP_gtk

# Client UDP avec GTK
clientUDP_gtk: clientUDP_gtk.c common.h
	$(CC) $(CFLAGS) clientUDP_gtk.c -o clientUDP_gtk $(LIBS)
	@echo "Client UDP GTK compilé avec succès!"

# Serveur UDP avec GTK
serveurUDP_gtk: serveurUDP_gtk.c common.h
	$(CC) $(CFLAGS) serveurUDP_gtk.c -o serveurUDP_gtk $(LIBS)
	@echo "Serveur UDP GTK compilé avec succès!"

# Client TCP avec GTK
clientTCP_gtk: clientTCP_gtk.c common.h
	$(CC) $(CFLAGS) clientTCP_gtk.c -o clientTCP_gtk $(LIBS)
	@echo "Client TCP GTK compilé avec succès!"

# Serveur TCP avec GTK
serveurTCP_gtk: serveurTCP_gtk.c common.h
	$(CC) $(CFLAGS) serveurTCP_gtk.c -o serveurTCP_gtk $(LIBS)
	@echo "Serveur TCP GTK compilé avec succès!"

# Nettoyage
clean:
	rm -f clientUDP_gtk serveurUDP_gtk clientTCP_gtk serveurTCP_gtk
	@echo "Nettoyage terminé!"

# Aide
help:
	@echo "Commandes disponibles:"
	@echo "  make all    - Compile tous les programmes"
	@echo "  make udp    - Compile les programmes UDP"
	@echo "  make tcp    - Compile les programmes TCP"
	@echo "  make clean  - Supprime les fichiers compilés"
	@echo ""
	@echo "Programmes créés:"
	@echo "  clientUDP_gtk   - Client UDP avec interface GTK"
	@echo "  serveurUDP_gtk  - Serveur UDP avec interface GTK"
	@echo "  clientTCP_gtk   - Client TCP avec interface GTK"
	@echo "  serveurTCP_gtk  - Serveur TCP avec interface GTK"

.PHONY: all udp tcp clean help

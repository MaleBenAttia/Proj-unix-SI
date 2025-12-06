/*
 * Serveur UDP avec interface graphique GTK
 * Compilation: gcc serveurUDP_gtk.c -o serveurUDP_gtk `pkg-config --cflags --libs gtk+-3.0`
 * Utilisation: ./serveurUDP_gtk
 */

#include "common.h"
#include <gtk/gtk.h>

// Structure pour stocker les widgets et le socket
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_port;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *btn_demarrer;
    GtkWidget *btn_arreter;
    int sockfd;
    gboolean running;
} AppWidgets;

/*
 * Fonction callback pour traiter les requêtes UDP
 * Appelée périodiquement par GTK
 */
gboolean traiter_requetes(gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    if (!app->running) {
        return FALSE; // Arrêter le timer
    }
    
    struct sockaddr_in client_addr;
    int n;
    int buffer_send[BUFFER_SIZE];
    socklen_t len;
    fd_set readfds;
    struct timeval tv;
    
    // Configuration du timeout (non-bloquant)
    FD_ZERO(&readfds);
    FD_SET(app->sockfd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms
    
    // Vérifier s'il y a des données à lire
    int ret = select(app->sockfd + 1, &readfds, NULL, NULL, &tv);
    
    if (ret > 0) {
        // Recevoir n du client
        len = sizeof(client_addr);
        recvfrom(app->sockfd, &n, sizeof(int), 0, (struct sockaddr *)&client_addr, &len);
        
        // Afficher dans l'interface
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        
        char message[256];
        sprintf(message, "Nombre reçu: %d\n", n);
        gtk_text_buffer_insert(app->buffer, &iter, message, -1);
        
        // Générer n nombres aléatoires
        sprintf(message, "Génération de %d nombres: ", n);
        gtk_text_buffer_insert(app->buffer, &iter, message, -1);
        
        char nombres[1024] = "";
        for (int i = 0; i < n; i++) {
            buffer_send[i] = (rand() % MAX_RANDOM) + 1;
            char temp[20];
            sprintf(temp, "%d ", buffer_send[i]);
            strcat(nombres, temp);
        }
        strcat(nombres, "\n");
        gtk_text_buffer_insert(app->buffer, &iter, nombres, -1);
        
        // Envoyer les nombres au client
        sendto(app->sockfd, buffer_send, n * sizeof(int), 0, 
               (struct sockaddr *)&client_addr, len);
        gtk_text_buffer_insert(app->buffer, &iter, "Nombres envoyés au client\n\n", -1);
        
        // Auto-scroll vers le bas
        GtkTextMark *mark = gtk_text_buffer_get_insert(app->buffer);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->text_view), mark, 0.0, TRUE, 0.0, 1.0);
    }
    
    return TRUE; // Continuer le timer
}

/*
 * Fonction appelée lors du clic sur "Démarrer"
 */
void on_demarrer_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(app->entry_port));
    
    if (strlen(port_str) == 0) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        gtk_text_buffer_insert(app->buffer, &iter, "Erreur: Veuillez entrer un port\n", -1);
        return;
    }
    
    int port = atoi(port_str);
    struct sockaddr_in serveur_addr;
    
    // Créer le socket UDP
    app->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (app->sockfd < 0) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        gtk_text_buffer_insert(app->buffer, &iter, "Erreur: Impossible de créer le socket\n", -1);
        return;
    }
    
    // Configurer l'adresse du serveur
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    serveur_addr.sin_port = htons(port);
    
    // Lier le socket au port
    if (bind(app->sockfd, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr)) < 0) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        gtk_text_buffer_insert(app->buffer, &iter, "Erreur: Impossible de lier le socket\n", -1);
        close(app->sockfd);
        return;
    }
    
    // Initialiser le générateur aléatoire
    srand(time(NULL));
    
    // Afficher le message de démarrage
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app->buffer, &iter);
    char message[256];
    sprintf(message, "Serveur démarré sur le port %d\n", port);
    gtk_text_buffer_insert(app->buffer, &iter, message, -1);
    gtk_text_buffer_insert(app->buffer, &iter, "En attente de requêtes...\n\n", -1);
    
    // Activer le serveur
    app->running = TRUE;
    gtk_widget_set_sensitive(app->btn_demarrer, FALSE);
    gtk_widget_set_sensitive(app->btn_arreter, TRUE);
    gtk_widget_set_sensitive(app->entry_port, FALSE);
    
    // Démarrer le timer pour traiter les requêtes
    g_timeout_add(100, traiter_requetes, app);
}

/*
 * Fonction appelée lors du clic sur "Arrêter"
 */
void on_arreter_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    app->running = FALSE;
    close(app->sockfd);
    
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app->buffer, &iter);
    gtk_text_buffer_insert(app->buffer, &iter, "Serveur arrêté\n\n", -1);
    
    gtk_widget_set_sensitive(app->btn_demarrer, TRUE);
    gtk_widget_set_sensitive(app->btn_arreter, FALSE);
    gtk_widget_set_sensitive(app->entry_port, TRUE);
}

/*
 * Fonction appelée lors de la fermeture de la fenêtre
 */
void on_window_destroy(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    if (app->running) {
        close(app->sockfd);
    }
    gtk_main_quit();
}

/*
 * Fonction principale - création de l'interface
 */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets *app = g_malloc(sizeof(AppWidgets));
    app->running = FALSE;
    
    // Créer la fenêtre principale
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Serveur UDP");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 10);
    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), app);
    
    // Créer le conteneur vertical principal
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    
    // Section port
    GtkWidget *hbox_port = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_port = gtk_label_new("Port:");
    app->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_port), "5000");
    gtk_box_pack_start(GTK_BOX(hbox_port), label_port, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_port), app->entry_port, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_port, FALSE, FALSE, 0);
    
    // Boutons de contrôle
    GtkWidget *hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    app->btn_demarrer = gtk_button_new_with_label("Démarrer");
    app->btn_arreter = gtk_button_new_with_label("Arrêter");
    gtk_widget_set_sensitive(app->btn_arreter, FALSE);
    g_signal_connect(app->btn_demarrer, "clicked", G_CALLBACK(on_demarrer_clicked), app);
    g_signal_connect(app->btn_arreter, "clicked", G_CALLBACK(on_arreter_clicked), app);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), app->btn_demarrer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), app->btn_arreter, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);
    
    // Zone de texte pour afficher les logs
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    app->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->text_view), FALSE);
    app->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view));
    gtk_container_add(GTK_CONTAINER(scrolled), app->text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    // Afficher tous les widgets
    gtk_widget_show_all(app->window);
    
    // Lancer la boucle GTK
    gtk_main();
    
    g_free(app);
    return 0;
}

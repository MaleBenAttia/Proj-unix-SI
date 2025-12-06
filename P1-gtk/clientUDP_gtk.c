/*
 * Client UDP avec interface graphique GTK
 * Compilation: gcc clientUDP_gtk.c -o clientUDP_gtk `pkg-config --cflags --libs gtk+-3.0`
 * Utilisation: ./clientUDP_gtk
 */

#include "common.h"
#include <gtk/gtk.h>

// Structure pour stocker les widgets
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_serveur;
    GtkWidget *entry_port;
    GtkWidget *label_nombre;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *btn_envoyer;
} AppWidgets;

/*
 * Fonction appelée lors du clic sur "Envoyer"
 */
void on_envoyer_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    // Récupérer les valeurs des champs
    const char *serveur_nom = gtk_entry_get_text(GTK_ENTRY(app->entry_serveur));
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(app->entry_port));
    
    // Vérifier que les champs ne sont pas vides
    if (strlen(serveur_nom) == 0 || strlen(port_str) == 0) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        gtk_text_buffer_insert(app->buffer, &iter, "Erreur: Veuillez remplir tous les champs\n", -1);
        return;
    }
    
    int port = atoi(port_str);
    int sockfd;
    struct sockaddr_in serveur_addr;
    struct hostent *serveur;
    int n;
    int buffer_recv[BUFFER_SIZE];
    socklen_t len;
    
    // Créer le socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        gtk_text_buffer_insert(app->buffer, &iter, "Erreur: Impossible de créer le socket\n", -1);
        return;
    }
    
    // Obtenir l'adresse du serveur
    serveur = gethostbyname(serveur_nom);
    if (serveur == NULL) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(app->buffer, &iter);
        gtk_text_buffer_insert(app->buffer, &iter, "Erreur: Serveur inconnu\n", -1);
        close(sockfd);
        return;
    }
    
    // Configurer l'adresse du serveur
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(port);
    memcpy(&serveur_addr.sin_addr, serveur->h_addr, serveur->h_length);
    
    // Générer un nombre aléatoire
    srand(time(NULL));
    n = (rand() % NMAX) + 1;
    
    // Afficher le nombre généré
    char message[256];
    sprintf(message, "Nombre généré: %d\n", n);
    gtk_label_set_text(GTK_LABEL(app->label_nombre), message);
    
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app->buffer, &iter);
    gtk_text_buffer_insert(app->buffer, &iter, message, -1);
    
    // Envoyer n au serveur
    len = sizeof(serveur_addr);
    sendto(sockfd, &n, sizeof(int), 0, (struct sockaddr *)&serveur_addr, len);
    gtk_text_buffer_insert(app->buffer, &iter, "Nombre envoyé au serveur\n", -1);
    
    // Recevoir la réponse
    recvfrom(sockfd, buffer_recv, BUFFER_SIZE, 0, (struct sockaddr *)&serveur_addr, &len);
    
    // Afficher les nombres reçus
    gtk_text_buffer_get_end_iter(app->buffer, &iter);
    gtk_text_buffer_insert(app->buffer, &iter, "Nombres reçus du serveur: ", -1);
    
    char nombres[1024] = "";
    for (int i = 0; i < n; i++) {
        char temp[20];
        sprintf(temp, "%d ", buffer_recv[i]);
        strcat(nombres, temp);
    }
    strcat(nombres, "\n\n");
    gtk_text_buffer_insert(app->buffer, &iter, nombres, -1);
    
    // Fermer le socket
    close(sockfd);
}

/*
 * Fonction appelée lors de la fermeture de la fenêtre
 */
void on_window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

/*
 * Fonction principale - création de l'interface
 */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets *app = g_malloc(sizeof(AppWidgets));
    
    // Créer la fenêtre principale
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Client UDP");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 10);
    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // Créer le conteneur vertical principal
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    
    // Section serveur
    GtkWidget *hbox_serveur = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_serveur = gtk_label_new("Serveur:");
    app->entry_serveur = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_serveur), "localhost");
    gtk_box_pack_start(GTK_BOX(hbox_serveur), label_serveur, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_serveur), app->entry_serveur, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_serveur, FALSE, FALSE, 0);
    
    // Section port
    GtkWidget *hbox_port = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_port = gtk_label_new("Port:");
    app->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_port), "5000");
    gtk_box_pack_start(GTK_BOX(hbox_port), label_port, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_port), app->entry_port, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_port, FALSE, FALSE, 0);
    
    // Label pour afficher le nombre généré
    app->label_nombre = gtk_label_new("En attente...");
    gtk_box_pack_start(GTK_BOX(vbox), app->label_nombre, FALSE, FALSE, 0);
    
    // Bouton envoyer
    app->btn_envoyer = gtk_button_new_with_label("Envoyer");
    g_signal_connect(app->btn_envoyer, "clicked", G_CALLBACK(on_envoyer_clicked), app);
    gtk_box_pack_start(GTK_BOX(vbox), app->btn_envoyer, FALSE, FALSE, 0);
    
    // Zone de texte pour afficher les résultats
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

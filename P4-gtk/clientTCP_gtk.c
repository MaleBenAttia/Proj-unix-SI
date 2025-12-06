/*
 * ============================================
 *   CLIENT TCP MULTISERVICE AVEC GTK
 * ============================================
 * 
 * Compilation:
 *   gcc clientTCP_gtk.c -o clientTCP_gtk `pkg-config --cflags --libs gtk+-3.0`
 * 
 * Services disponibles:
 *   [1] Date et Heure du serveur
 *   [2] Liste des fichiers d'un répertoire
 *   [3] Contenu d'un fichier
 *   [4] Durée de connexion
 *   [0] Quitter
 */

#include "common.h"
#include <gtk/gtk.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>

// Structure pour l'application
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_serveur;
    GtkWidget *entry_port;
    GtkWidget *entry_username;
    GtkWidget *entry_password;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *btn_connecter;
    GtkWidget *btn_date;
    GtkWidget *btn_liste;
    GtkWidget *btn_contenu;
    GtkWidget *btn_duree;
    GtkWidget *btn_deconnecter;
    int sockfd;
    gboolean connected;
} AppWidgets;

/*
 * Fonction: afficher_message
 * --------------------------
 * Affiche un message dans la zone de texte
 */
void afficher_message(AppWidgets *app, const char *message) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app->buffer, &iter);
    gtk_text_buffer_insert(app->buffer, &iter, message, -1);
    
    // Auto-scroll
    GtkTextMark *mark = gtk_text_buffer_get_insert(app->buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->text_view), 
                                 mark, 0.0, TRUE, 0.0, 1.0);
}

/*
 * SERVICE 1: Date et Heure
 */
void on_date_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    if (!app->connected) {
        afficher_message(app, "❌ Non connecté!\n\n");
        return;
    }
    
    int choix = SERVICE_DATE;
    char buffer[BUFFER_SIZE];
    
    // Envoyer le choix
    ssize_t n = write(app->sockfd, &choix, sizeof(int));
    if (n < 0) {
        afficher_message(app, "❌ Erreur d'envoi\n\n");
        return;
    }
    
    // Recevoir la réponse
    memset(buffer, 0, BUFFER_SIZE);
    n = read(app->sockfd, buffer, BUFFER_SIZE);
    if (n < 0) {
        afficher_message(app, "❌ Erreur de réception\n\n");
        return;
    }
    
    char message[BUFFER_SIZE + 100];
    sprintf(message, "\n📅 %s\n\n", buffer);
    afficher_message(app, message);
}

/*
 * SERVICE 2: Liste des fichiers
 */
void on_liste_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    // Demander le chemin
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_OK_CANCEL,
                                              "Entrez le chemin du répertoire:");
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), ".");
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);
    
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_OK) {
        const char *chemin = gtk_entry_get_text(GTK_ENTRY(entry));
        
        // Envoyer le choix
        int choix = SERVICE_LISTE_FICHIERS;
        write(app->sockfd, &choix, sizeof(int));
        
        // Envoyer le chemin
        write(app->sockfd, chemin, strlen(chemin) + 1);
        
        // Recevoir la liste
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        read(app->sockfd, buffer, BUFFER_SIZE);
        
        char message[BUFFER_SIZE + 100];
        sprintf(message, "\n📁 LISTE DES FICHIERS (%s):\n%s\n", chemin, buffer);
        afficher_message(app, message);
    }
    
    gtk_widget_destroy(dialog);
}

/*
 * SERVICE 3: Contenu d'un fichier
 */
void on_contenu_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    // Demander le nom du fichier
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_OK_CANCEL,
                                              "Entrez le nom du fichier:");
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "exemple.txt");
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);
    
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_OK) {
        const char *fichier = gtk_entry_get_text(GTK_ENTRY(entry));
        
        // Envoyer le choix
        int choix = SERVICE_CONTENU_FICHIER;
        write(app->sockfd, &choix, sizeof(int));
        
        // Envoyer le nom
        write(app->sockfd, fichier, strlen(fichier) + 1);
        
        // Recevoir le contenu
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        read(app->sockfd, buffer, BUFFER_SIZE);
        
        char message[BUFFER_SIZE + 100];
        sprintf(message, "\n📄 CONTENU DU FICHIER (%s):\n%s\n", fichier, buffer);
        afficher_message(app, message);
    }
    
    gtk_widget_destroy(dialog);
}

/*
 * SERVICE 4: Durée de connexion
 */
void on_duree_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    int choix = SERVICE_DUREE_CONNEXION;
    char buffer[BUFFER_SIZE];
    
    // Envoyer le choix
    write(app->sockfd, &choix, sizeof(int));
    
    // Recevoir la réponse
    memset(buffer, 0, BUFFER_SIZE);
    read(app->sockfd, buffer, BUFFER_SIZE);
    
    char message[BUFFER_SIZE + 100];
    sprintf(message, "\n⏱️  %s\n\n", buffer);
    afficher_message(app, message);
}

/*
 * Fonction: on_deconnecter_clicked
 */
void on_deconnecter_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    int choix = SERVICE_FIN;
    write(app->sockfd, &choix, sizeof(int));
    close(app->sockfd);
    
    app->connected = FALSE;
    gtk_widget_set_sensitive(app->btn_connecter, TRUE);
    gtk_widget_set_sensitive(app->btn_date, FALSE);
    gtk_widget_set_sensitive(app->btn_liste, FALSE);
    gtk_widget_set_sensitive(app->btn_contenu, FALSE);
    gtk_widget_set_sensitive(app->btn_duree, FALSE);
    gtk_widget_set_sensitive(app->btn_deconnecter, FALSE);
    gtk_widget_set_sensitive(app->entry_serveur, TRUE);
    gtk_widget_set_sensitive(app->entry_port, TRUE);
    gtk_widget_set_sensitive(app->entry_username, TRUE);
    gtk_widget_set_sensitive(app->entry_password, TRUE);
    
    afficher_message(app, "🔌 Déconnecté du serveur\n\n");
}

/*
 * Fonction: on_connecter_clicked
 * ------------------------------
 * Connexion et authentification
 */
void on_connecter_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    // Récupérer les valeurs
    const char *serveur_nom = gtk_entry_get_text(GTK_ENTRY(app->entry_serveur));
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(app->entry_port));
    const char *username = gtk_entry_get_text(GTK_ENTRY(app->entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(app->entry_password));
    
    if (strlen(serveur_nom) == 0 || strlen(port_str) == 0 || 
        strlen(username) == 0 || strlen(password) == 0) {
        afficher_message(app, "❌ Erreur: Veuillez remplir tous les champs\n\n");
        return;
    }
    
    int port = atoi(port_str);
    
    // Ignorer SIGPIPE pour éviter que le programme plante
    signal(SIGPIPE, SIG_IGN);
    
    // Créer le socket TCP
    app->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (app->sockfd < 0) {
        afficher_message(app, "❌ Erreur: Impossible de créer le socket\n\n");
        return;
    }
    
    // Définir un timeout pour les opérations
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 secondes
    timeout.tv_usec = 0;
    setsockopt(app->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(app->sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Obtenir l'adresse du serveur
    struct hostent *serveur = gethostbyname(serveur_nom);
    if (serveur == NULL) {
        afficher_message(app, "❌ Erreur: Serveur inconnu\n\n");
        close(app->sockfd);
        return;
    }
    
    // Configurer l'adresse
    struct sockaddr_in serveur_addr;
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(port);
    memcpy(&serveur_addr.sin_addr, serveur->h_addr, serveur->h_length);
    
    // Se connecter
    afficher_message(app, "🔄 Connexion au serveur...\n");
    if (connect(app->sockfd, (struct sockaddr *)&serveur_addr, 
                sizeof(serveur_addr)) < 0) {
        char error_msg[256];
        sprintf(error_msg, "❌ Erreur: Connexion impossible (%s)\n\n", 
                strerror(errno));
        afficher_message(app, error_msg);
        close(app->sockfd);
        return;
    }
    
    afficher_message(app, "✅ Connecté au serveur!\n\n");
    
    // AUTHENTIFICATION
    afficher_message(app, "🔐 AUTHENTIFICATION...\n");
    
    // Copier les données dans des buffers locaux
    char user_buffer[256];
    char pass_buffer[256];
    strncpy(user_buffer, username, sizeof(user_buffer) - 1);
    strncpy(pass_buffer, password, sizeof(pass_buffer) - 1);
    user_buffer[255] = '\0';
    pass_buffer[255] = '\0';
    
    // Envoyer username
    ssize_t n = write(app->sockfd, user_buffer, strlen(user_buffer) + 1);
    if (n < 0) {
        char error_msg[256];
        sprintf(error_msg, "❌ Erreur d'envoi du username (%s)\n\n", 
                strerror(errno));
        afficher_message(app, error_msg);
        close(app->sockfd);
        return;
    }
    afficher_message(app, "   ✓ Username envoyé\n");
    
    // Petit délai
    usleep(100000); // 100ms
    
    // Envoyer password
    n = write(app->sockfd, pass_buffer, strlen(pass_buffer) + 1);
    if (n < 0) {
        char error_msg[256];
        sprintf(error_msg, "❌ Erreur d'envoi du password (%s)\n\n", 
                strerror(errno));
        afficher_message(app, error_msg);
        close(app->sockfd);
        return;
    }
    afficher_message(app, "   ✓ Password envoyé\n");
    afficher_message(app, "   ⏳ Attente de la réponse...\n");
    
    // Recevoir le résultat
    int resultat;
    n = read(app->sockfd, &resultat, sizeof(int));
    if (n != sizeof(int)) {
        char error_msg[256];
        sprintf(error_msg, "❌ Erreur de réception (reçu %zd octets, attendu %zu) - %s\n\n", 
                n, sizeof(int), n < 0 ? strerror(errno) : "timeout ou connexion fermée");
        afficher_message(app, error_msg);
        close(app->sockfd);
        return;
    }
    
    if (resultat == 0) {
        afficher_message(app, "❌ Échec de l'authentification!\n\n");
        close(app->sockfd);
        return;
    }
    
    afficher_message(app, "✅ Authentification OK!\n\n");
    afficher_message(app, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    afficher_message(app, "📋 Services disponibles:\n");
    afficher_message(app, "   [1] Date et Heure\n");
    afficher_message(app, "   [2] Liste des fichiers\n");
    afficher_message(app, "   [3] Contenu d'un fichier\n");
    afficher_message(app, "   [4] Durée de connexion\n");
    afficher_message(app, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    // Activer l'interface
    app->connected = TRUE;
    gtk_widget_set_sensitive(app->btn_connecter, FALSE);
    gtk_widget_set_sensitive(app->btn_date, TRUE);
    gtk_widget_set_sensitive(app->btn_liste, TRUE);
    gtk_widget_set_sensitive(app->btn_contenu, TRUE);
    gtk_widget_set_sensitive(app->btn_duree, TRUE);
    gtk_widget_set_sensitive(app->btn_deconnecter, TRUE);
    gtk_widget_set_sensitive(app->entry_serveur, FALSE);
    gtk_widget_set_sensitive(app->entry_port, FALSE);
    gtk_widget_set_sensitive(app->entry_username, FALSE);
    gtk_widget_set_sensitive(app->entry_password, FALSE);
}

/*
 * Fonction: on_window_destroy
 */
void on_window_destroy(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    if (app->connected) {
        int choix = SERVICE_FIN;
        write(app->sockfd, &choix, sizeof(int));
        close(app->sockfd);
    }
    gtk_main_quit();
}

/*
 * Fonction: main
 * -------------
 * Création de l'interface
 */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets *app = g_malloc(sizeof(AppWidgets));
    app->connected = FALSE;
    
    // FENÊTRE PRINCIPALE
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), 
                        "🌐 Client TCP Multiservice");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 700, 600);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 15);
    g_signal_connect(app->window, "destroy", 
                     G_CALLBACK(on_window_destroy), app);
    
    // BOÎTE VERTICALE
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    
    // TITRE
    GtkWidget *label_titre = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_titre), 
        "<span font='16' weight='bold'>🌐 Client TCP Multiservice</span>");
    gtk_box_pack_start(GTK_BOX(vbox), label_titre, FALSE, FALSE, 5);
    
    // SÉPARATEUR
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 5);
    
    // SECTION CONNEXION
    GtkWidget *label_config = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_config), 
        "<b>🔧 Configuration</b>");
    gtk_widget_set_halign(label_config, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_config, FALSE, FALSE, 0);
    
    // Serveur
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl1 = gtk_label_new("🖥️  Serveur:");
    gtk_widget_set_size_request(lbl1, 100, -1);
    app->entry_serveur = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_serveur), "localhost");
    gtk_box_pack_start(GTK_BOX(hbox1), lbl1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), app->entry_serveur, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    
    // Port
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl2 = gtk_label_new("🔌 Port:");
    gtk_widget_set_size_request(lbl2, 100, -1);
    app->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_port), "6000");
    gtk_box_pack_start(GTK_BOX(hbox2), lbl2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), app->entry_port, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    
    // Username
    GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl3 = gtk_label_new("👤 Username:");
    gtk_widget_set_size_request(lbl3, 100, -1);
    app->entry_username = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_username), "admin");
    gtk_box_pack_start(GTK_BOX(hbox3), lbl3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), app->entry_username, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 0);
    
    // Password
    GtkWidget *hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl4 = gtk_label_new("🔒 Password:");
    gtk_widget_set_size_request(lbl4, 100, -1);
    app->entry_password = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_password), "admin123");
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_password), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox4), lbl4, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox4), app->entry_password, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox4, FALSE, FALSE, 0);
    
    // Bouton connecter
    app->btn_connecter = gtk_button_new_with_label("🔗 Connecter");
    g_signal_connect(app->btn_connecter, "clicked", 
                     G_CALLBACK(on_connecter_clicked), app);
    gtk_box_pack_start(GTK_BOX(vbox), app->btn_connecter, FALSE, FALSE, 0);
    
    // SÉPARATEUR
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 5);
    
    // SECTION SERVICES
    GtkWidget *label_services = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_services), 
        "<b>📋 Services disponibles</b>");
    gtk_widget_set_halign(label_services, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_services, FALSE, FALSE, 0);
    
    // Boutons services - Ligne 1
    GtkWidget *hbox_srv1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    app->btn_date = gtk_button_new_with_label("📅 Date/Heure");
    app->btn_liste = gtk_button_new_with_label("📁 Liste fichiers");
    gtk_widget_set_sensitive(app->btn_date, FALSE);
    gtk_widget_set_sensitive(app->btn_liste, FALSE);
    g_signal_connect(app->btn_date, "clicked", 
                     G_CALLBACK(on_date_clicked), app);
    g_signal_connect(app->btn_liste, "clicked", 
                     G_CALLBACK(on_liste_clicked), app);
    gtk_box_pack_start(GTK_BOX(hbox_srv1), app->btn_date, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_srv1), app->btn_liste, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_srv1, FALSE, FALSE, 0);
    
    // Boutons services - Ligne 2
    GtkWidget *hbox_srv2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    app->btn_contenu = gtk_button_new_with_label("📄 Contenu fichier");
    app->btn_duree = gtk_button_new_with_label("⏱️  Durée connexion");
    gtk_widget_set_sensitive(app->btn_contenu, FALSE);
    gtk_widget_set_sensitive(app->btn_duree, FALSE);
    g_signal_connect(app->btn_contenu, "clicked", 
                     G_CALLBACK(on_contenu_clicked), app);
    g_signal_connect(app->btn_duree, "clicked", 
                     G_CALLBACK(on_duree_clicked), app);
    gtk_box_pack_start(GTK_BOX(hbox_srv2), app->btn_contenu, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_srv2), app->btn_duree, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_srv2, FALSE, FALSE, 0);
    
    // Bouton déconnecter
    app->btn_deconnecter = gtk_button_new_with_label("🔌 Déconnecter");
    gtk_widget_set_sensitive(app->btn_deconnecter, FALSE);
    g_signal_connect(app->btn_deconnecter, "clicked", 
                     G_CALLBACK(on_deconnecter_clicked), app);
    gtk_box_pack_start(GTK_BOX(vbox), app->btn_deconnecter, FALSE, FALSE, 0);
    
    // SÉPARATEUR
    GtkWidget *sep3 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep3, FALSE, FALSE, 5);
    
    // ZONE DE TEXTE
    GtkWidget *label_resultats = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_resultats), 
        "<b>📊 Résultats</b>");
    gtk_widget_set_halign(label_resultats, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_resultats, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                   GTK_POLICY_AUTOMATIC, 
                                   GTK_POLICY_AUTOMATIC);
    
    app->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->text_view), GTK_WRAP_WORD);
    app->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view));
    
    gtk_container_add(GTK_CONTAINER(scrolled), app->text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    // AFFICHER
    gtk_widget_show_all(app->window);
    gtk_main();
    
    g_free(app);
    return 0;
}

/*
 * ============================================
 *  SERVEUR TCP MULTISERVICE MULTI-PORT GTK
 * ============================================
 * 
 * Compilation:
 *   gcc serveurTCP_gtk.c -o serveurTCP_gtk `pkg-config --cflags --libs gtk+-3.0`
 * 
 * Fonctionnalités:
 *   - Chaque service sur un port différent
 *   - Support multiclient
 *   - Interface graphique pour les logs
 */

#include "common.h"
#include <gtk/gtk.h>
#include <pthread.h>

// Structure pour l'application
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_username;
    GtkWidget *entry_password;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *btn_demarrer;
    GtkWidget *btn_arreter;
    int sockfd_auth;
    int sockfd_date;
    int sockfd_liste;
    int sockfd_contenu;
    int sockfd_duree;
    gboolean running;
    pthread_t threads[5];
} AppWidgets;

// Variables globales
static AppWidgets *global_app = NULL;
static char global_username[50];
static char global_password[50];
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Fonction: afficher_log (thread-safe)
 */
gboolean afficher_log_idle(gpointer data) {
    char *message = (char *)data;
    
    if (global_app && global_app->buffer) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(global_app->buffer, &iter);
        gtk_text_buffer_insert(global_app->buffer, &iter, message, -1);
        
        GtkTextMark *mark = gtk_text_buffer_get_insert(global_app->buffer);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(global_app->text_view), 
                                     mark, 0.0, TRUE, 0.0, 1.0);
    }
    
    g_free(message);
    return FALSE;
}

void afficher_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    char *msg_copy = g_strdup(message);
    g_idle_add(afficher_log_idle, msg_copy);
    pthread_mutex_unlock(&log_mutex);
}

/*
 * Créer un socket serveur
 */
int creer_socket_serveur(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 5) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/*
 * SERVICE 1: Date et Heure
 */
void* thread_service_date(void* arg) {
    AppWidgets *app = (AppWidgets *)arg;
    
    while (app->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(app->sockfd_date, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        afficher_log("📅 [DATE] Client connecté\n");
        
        char buffer[BUFFER_SIZE];
        time_t now;
        struct tm *timeinfo;
        
        time(&now);
        timeinfo = localtime(&now);
        strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", timeinfo);
        
        write(client_sock, buffer, strlen(buffer) + 1);
        afficher_log("   ✅ Date/Heure envoyée\n");
        
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 2: Liste des fichiers
 */
void* thread_service_liste(void* arg) {
    AppWidgets *app = (AppWidgets *)arg;
    
    while (app->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(app->sockfd_liste, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        afficher_log("📂 [LISTE] Client connecté\n");
        
        char chemin[256];
        char buffer[BUFFER_SIZE];
        
        memset(chemin, 0, sizeof(chemin));
        read(client_sock, chemin, sizeof(chemin));
        
        DIR *dir = opendir(chemin);
        if (dir == NULL) {
            sprintf(buffer, "Erreur: Impossible d'ouvrir le répertoire");
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                strcat(buffer, entry->d_name);
                strcat(buffer, "\n");
            }
            closedir(dir);
        }
        
        write(client_sock, buffer, strlen(buffer) + 1);
        afficher_log("   ✅ Liste envoyée\n");
        
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 3: Contenu fichier
 */
void* thread_service_contenu(void* arg) {
    AppWidgets *app = (AppWidgets *)arg;
    
    while (app->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(app->sockfd_contenu, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        afficher_log("📄 [CONTENU] Client connecté\n");
        
        char nom_fichier[256];
        char buffer[BUFFER_SIZE];
        
        memset(nom_fichier, 0, sizeof(nom_fichier));
        read(client_sock, nom_fichier, sizeof(nom_fichier));
        
        FILE *file = fopen(nom_fichier, "r");
        if (file == NULL) {
            sprintf(buffer, "Erreur: Impossible d'ouvrir le fichier");
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            int n = fread(buffer, 1, BUFFER_SIZE - 1, file);
            buffer[n] = '\0';
            fclose(file);
        }
        
        write(client_sock, buffer, strlen(buffer) + 1);
        afficher_log("   ✅ Contenu envoyé\n");
        
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE 4: Durée connexion
 */
void* thread_service_duree(void* arg) {
    AppWidgets *app = (AppWidgets *)arg;
    
    while (app->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(app->sockfd_duree, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        afficher_log("⏱️ [DURÉE] Client connecté\n");
        
        time_t debut;
        read(client_sock, &debut, sizeof(time_t));
        
        char buffer[BUFFER_SIZE];
        time_t maintenant;
        time(&maintenant);
        int duree_sec = (int)difftime(maintenant, debut);
        int minutes = duree_sec / 60;
        int secondes = duree_sec % 60;
        
        sprintf(buffer, "Durée de connexion: %d minute(s) et %d seconde(s)", 
                minutes, secondes);
        
        write(client_sock, buffer, strlen(buffer) + 1);
        afficher_log("   ✅ Durée envoyée\n");
        
        close(client_sock);
    }
    
    return NULL;
}

/*
 * SERVICE AUTH: Authentification
 */
void* thread_service_auth(void* arg) {
    AppWidgets *app = (AppWidgets *)arg;
    
    while (app->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(app->sockfd_auth, 
                                (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        afficher_log("🔐 [AUTH] Nouveau client\n");
        
        char username[50], password[50];
        
        memset(username, 0, sizeof(username));
        read(client_sock, username, sizeof(username));
        
        memset(password, 0, sizeof(password));
        read(client_sock, password, sizeof(password));
        
        int auth_ok;
        if (strcmp(username, global_username) == 0 && 
            strcmp(password, global_password) == 0) {
            auth_ok = AUTH_SUCCESS;
            afficher_log("   ✅ Authentification OK\n");
        } else {
            auth_ok = AUTH_FAILURE;
            afficher_log("   ❌ Authentification ÉCHEC\n");
        }
        
        write(client_sock, &auth_ok, sizeof(int));
        close(client_sock);
    }
    
    return NULL;
}

/*
 * Démarrer le serveur
 */
void on_demarrer_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    const char *username = gtk_entry_get_text(GTK_ENTRY(app->entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(app->entry_password));
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        afficher_log("❌ Erreur: Veuillez remplir tous les champs\n\n");
        return;
    }
    
    strcpy(global_username, username);
    strcpy(global_password, password);
    
    // Créer les sockets
    app->sockfd_auth = creer_socket_serveur(PORT_AUTH);
    app->sockfd_date = creer_socket_serveur(PORT_DATE);
    app->sockfd_liste = creer_socket_serveur(PORT_LISTE);
    app->sockfd_contenu = creer_socket_serveur(PORT_CONTENU);
    app->sockfd_duree = creer_socket_serveur(PORT_DUREE);
    
    if (app->sockfd_auth < 0 || app->sockfd_date < 0 || 
        app->sockfd_liste < 0 || app->sockfd_contenu < 0 || 
        app->sockfd_duree < 0) {
        afficher_log("❌ Erreur: Impossible de créer les sockets\n\n");
        return;
    }
    
    // Afficher les infos
    char message[512];
    sprintf(message, "╔═══════════════════════════════════════╗\n");
    afficher_log(message);
    sprintf(message, "║   SERVEUR TCP MULTISERVICE DÉMARRÉ    ║\n");
    afficher_log(message);
    sprintf(message, "╚═══════════════════════════════════════╝\n");
    afficher_log(message);
    sprintf(message, "👤 Compte: %s / %s\n", username, password);
    afficher_log(message);
    afficher_log("📋 Services disponibles:\n");
    sprintf(message, "   🔐 Auth:     Port %d\n", PORT_AUTH);
    afficher_log(message);
    sprintf(message, "   📅 Date:     Port %d\n", PORT_DATE);
    afficher_log(message);
    sprintf(message, "   📂 Liste:    Port %d\n", PORT_LISTE);
    afficher_log(message);
    sprintf(message, "   📄 Contenu:  Port %d\n", PORT_CONTENU);
    afficher_log(message);
    sprintf(message, "   ⏱️ Durée:    Port %d\n", PORT_DUREE);
    afficher_log(message);
    afficher_log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    afficher_log("👂 En attente de clients...\n\n");
    
    app->running = TRUE;
    
    // Lancer les threads
    pthread_create(&app->threads[0], NULL, thread_service_auth, app);
    pthread_create(&app->threads[1], NULL, thread_service_date, app);
    pthread_create(&app->threads[2], NULL, thread_service_liste, app);
    pthread_create(&app->threads[3], NULL, thread_service_contenu, app);
    pthread_create(&app->threads[4], NULL, thread_service_duree, app);
    
    gtk_widget_set_sensitive(app->btn_demarrer, FALSE);
    gtk_widget_set_sensitive(app->btn_arreter, TRUE);
    gtk_widget_set_sensitive(app->entry_username, FALSE);
    gtk_widget_set_sensitive(app->entry_password, FALSE);
}

/*
 * Arrêter le serveur
 */
void on_arreter_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    app->running = FALSE;
    
    // Fermer les sockets
    if (app->sockfd_auth >= 0) { shutdown(app->sockfd_auth, SHUT_RDWR); close(app->sockfd_auth); }
    if (app->sockfd_date >= 0) { shutdown(app->sockfd_date, SHUT_RDWR); close(app->sockfd_date); }
    if (app->sockfd_liste >= 0) { shutdown(app->sockfd_liste, SHUT_RDWR); close(app->sockfd_liste); }
    if (app->sockfd_contenu >= 0) { shutdown(app->sockfd_contenu, SHUT_RDWR); close(app->sockfd_contenu); }
    if (app->sockfd_duree >= 0) { shutdown(app->sockfd_duree, SHUT_RDWR); close(app->sockfd_duree); }
    
    afficher_log("\n🛑 Serveur arrêté\n\n");
    
    gtk_widget_set_sensitive(app->btn_demarrer, TRUE);
    gtk_widget_set_sensitive(app->btn_arreter, FALSE);
    gtk_widget_set_sensitive(app->entry_username, TRUE);
    gtk_widget_set_sensitive(app->entry_password, TRUE);
}

void on_window_destroy(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    if (app->running) {
        on_arreter_clicked(NULL, app);
    }
    
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets *app = g_malloc(sizeof(AppWidgets));
    app->running = FALSE;
    global_app = app;
    
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "🖥️ Serveur TCP Multiservice Multi-Port");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 700, 600);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 15);
    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), app);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    
    GtkWidget *label_titre = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_titre), 
        "<span font='16' weight='bold'>🖥️ Serveur TCP Multi-Port</span>");
    gtk_box_pack_start(GTK_BOX(vbox), label_titre, FALSE, FALSE, 5);
    
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 5);
    
    GtkWidget *label_config = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_config), "<b>🔧 Configuration</b>");
    gtk_widget_set_halign(label_config, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_config, FALSE, FALSE, 0);
    
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl1 = gtk_label_new("👤 Username:");
    gtk_widget_set_size_request(lbl1, 100, -1);
    app->entry_username = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_username), "admin");
    gtk_box_pack_start(GTK_BOX(hbox1), lbl1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), app->entry_username, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl2 = gtk_label_new("🔑 Password:");
    gtk_widget_set_size_request(lbl2, 100, -1);
    app->entry_password = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_password), "admin123");
    gtk_box_pack_start(GTK_BOX(hbox2), lbl2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), app->entry_password, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    
    GtkWidget *hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    app->btn_demarrer = gtk_button_new_with_label("▶️ Démarrer");
    app->btn_arreter = gtk_button_new_with_label("⏹️ Arrêter");
    gtk_widget_set_sensitive(app->btn_arreter, FALSE);
    g_signal_connect(app->btn_demarrer, "clicked", G_CALLBACK(on_demarrer_clicked), app);
    g_signal_connect(app->btn_arreter, "clicked", G_CALLBACK(on_arreter_clicked), app);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), app->btn_demarrer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), app->btn_arreter, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);
    
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 5);
    
    GtkWidget *label_logs = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_logs), "<b>📋 Logs du serveur</b>");
    gtk_widget_set_halign(label_logs, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_logs, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    app->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->text_view), GTK_WRAP_WORD);
    app->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view));
    
    gtk_container_add(GTK_CONTAINER(scrolled), app->text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    gtk_widget_show_all(app->window);
    gtk_main();
    
    g_free(app);
    return 0;
}

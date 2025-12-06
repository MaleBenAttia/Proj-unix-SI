/*
 * ============================================
 *  SERVEUR TCP MULTISERVICE/MULTICLIENT GTK
 * ============================================
 * 
 * Compilation:
 *   gcc serveurTCP_gtk.c -o serveurTCP_gtk `pkg-config --cflags --libs gtk+-3.0`
 * 
 * Fonctionnalités:
 *   - Support multiclient (via fork)
 *   - 4 services disponibles
 *   - Interface graphique pour les logs
 */

#include "common.h"
#include <gtk/gtk.h>

// Structure pour l'application
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_port;
    GtkWidget *entry_username;
    GtkWidget *entry_password;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *btn_demarrer;
    GtkWidget *btn_arreter;
    int sockfd;
    gboolean running;
    GThread *thread_serveur;
} AppWidgets;

// Variables globales partagées
static AppWidgets *global_app = NULL;
static char global_username[50];
static char global_password[50];

/*
 * Fonction: afficher_log
 * ----------------------
 * Affiche un message dans l'interface (thread-safe)
 */
gboolean afficher_log_idle(gpointer data) {
    char *message = (char *)data;
    
    if (global_app && global_app->buffer) {
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(global_app->buffer, &iter);
        gtk_text_buffer_insert(global_app->buffer, &iter, message, -1);
        
        // Auto-scroll
        GtkTextMark *mark = gtk_text_buffer_get_insert(global_app->buffer);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(global_app->text_view), 
                                     mark, 0.0, TRUE, 0.0, 1.0);
    }
    
    g_free(message);
    return FALSE;
}

void afficher_log(const char *message) {
    char *msg_copy = g_strdup(message);
    g_idle_add(afficher_log_idle, msg_copy);
}

/*
 * Gestionnaire de signal SIGCHLD
 */
void gestionnaire_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 * SERVICE 1: Date et Heure
 */
void service_date(int client_sockfd) {
    char buffer[BUFFER_SIZE];
    time_t now;
    struct tm *timeinfo;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(buffer, BUFFER_SIZE, "Date: %d/%m/%Y - Heure: %H:%M:%S", 
             timeinfo);
    
    write(client_sockfd, buffer, strlen(buffer) + 1);
    
    printf("   📅 Service DATE envoyé\n");
    fflush(stdout);
}

/*
 * SERVICE 2: Liste des fichiers
 */
void service_liste_fichiers(int client_sockfd) {
    char chemin[256];
    char buffer[BUFFER_SIZE];
    DIR *dir;
    struct dirent *entry;
    
    // Recevoir le chemin
    memset(chemin, 0, sizeof(chemin));
    read(client_sockfd, chemin, sizeof(chemin));
    
    printf("   📁 Liste fichiers de: %s\n", chemin);
    fflush(stdout);
    
    // Ouvrir le répertoire
    dir = opendir(chemin);
    if (dir == NULL) {
        sprintf(buffer, "Erreur: Impossible d'ouvrir le répertoire");
        write(client_sockfd, buffer, strlen(buffer) + 1);
        return;
    }
    
    // Lire tous les fichiers
    memset(buffer, 0, BUFFER_SIZE);
    while ((entry = readdir(dir)) != NULL) {
        strcat(buffer, entry->d_name);
        strcat(buffer, "\n");
    }
    closedir(dir);
    
    // Envoyer la liste
    write(client_sockfd, buffer, strlen(buffer) + 1);
    printf("   ✅ Liste envoyée\n");
    fflush(stdout);
}

/*
 * SERVICE 3: Contenu d'un fichier
 */
void service_contenu_fichier(int client_sockfd) {
    char nom_fichier[256];
    char buffer[BUFFER_SIZE];
    FILE *file;
    int n;
    
    // Recevoir le nom du fichier
    memset(nom_fichier, 0, sizeof(nom_fichier));
    read(client_sockfd, nom_fichier, sizeof(nom_fichier));
    
    printf("   📄 Contenu fichier: %s\n", nom_fichier);
    fflush(stdout);
    
    // Ouvrir le fichier
    file = fopen(nom_fichier, "r");
    if (file == NULL) {
        sprintf(buffer, "Erreur: Impossible d'ouvrir le fichier");
        write(client_sockfd, buffer, strlen(buffer) + 1);
        return;
    }
    
    // Lire le contenu
    memset(buffer, 0, BUFFER_SIZE);
    n = fread(buffer, 1, BUFFER_SIZE - 1, file);
    buffer[n] = '\0';
    fclose(file);
    
    // Envoyer le contenu
    write(client_sockfd, buffer, strlen(buffer) + 1);
    
    printf("   ✅ Contenu envoyé (%d octets)\n", n);
    fflush(stdout);
}

/*
 * SERVICE 4: Durée de connexion
 */
void service_duree_connexion(int client_sockfd, time_t debut) {
    char buffer[BUFFER_SIZE];
    time_t maintenant;
    int duree_sec, minutes, secondes;
    
    time(&maintenant);
    duree_sec = (int)difftime(maintenant, debut);
    minutes = duree_sec / 60;
    secondes = duree_sec % 60;
    
    sprintf(buffer, "Durée de connexion: %d minute(s) et %d seconde(s)", 
            minutes, secondes);
    
    write(client_sockfd, buffer, strlen(buffer) + 1);
    
    printf("   ⏱️  Durée: %d:%02d\n", minutes, secondes);
    fflush(stdout);
}

/*
 * Fonction: traiter_client
 * ------------------------
 * Traite un client (dans un processus fils)
 */
void traiter_client(int client_sockfd) {
    char username[50], password[50];
    int choix;
    int auth_ok;
    time_t debut_connexion;
    
    // Le processus fils écrit directement sur stdout
    // qui sera capturé par le parent
    
    printf("🔵 Nouveau client (PID %d)\n", getpid());
    fflush(stdout);
    
    // Enregistrer l'heure de connexion
    time(&debut_connexion);

    // AUTHENTIFICATION
    memset(username, 0, sizeof(username));
    int n = read(client_sockfd, username, sizeof(username));
    if (n <= 0) {
        printf("   ❌ Erreur lecture username\n");
        fflush(stdout);
        close(client_sockfd);
        exit(0);
    }

    memset(password, 0, sizeof(password));
    n = read(client_sockfd, password, sizeof(password));
    if (n <= 0) {
        printf("   ❌ Erreur lecture password\n");
        fflush(stdout);
        close(client_sockfd);
        exit(0);
    }

    printf("   👤 Username: %s\n", username);
    fflush(stdout);

    // Vérifier
    if (strcmp(username, global_username) == 0 && 
        strcmp(password, global_password) == 0) {
        auth_ok = 1;
        printf("   ✅ Authentification OK\n");
    } else {
        auth_ok = 0;
        printf("   ❌ Authentification ÉCHEC\n");
    }
    fflush(stdout);

    // Envoyer le résultat
    n = write(client_sockfd, &auth_ok, sizeof(int));
    if (n <= 0) {
        printf("   ❌ Erreur envoi résultat auth\n");
        fflush(stdout);
        close(client_sockfd);
        exit(0);
    }

    if (auth_ok == 0) {
        close(client_sockfd);
        exit(0);
    }

    // BOUCLE DE SERVICE
    while (1) {
        // Recevoir le choix
        n = read(client_sockfd, &choix, sizeof(int));
        if (n <= 0) {
            printf("   ⚠️  Client déconnecté (connexion perdue)\n\n");
            fflush(stdout);
            break;
        }

        printf("   📋 Service demandé: %d\n", choix);
        fflush(stdout);

        switch(choix) {
            case SERVICE_FIN:
                printf("   👋 Client quitte\n\n");
                fflush(stdout);
                goto fin;
                
            case SERVICE_DATE:
                service_date(client_sockfd);
                break;
                
            case SERVICE_LISTE_FICHIERS:
                service_liste_fichiers(client_sockfd);
                break;
                
            case SERVICE_CONTENU_FICHIER:
                service_contenu_fichier(client_sockfd);
                break;
                
            case SERVICE_DUREE_CONNEXION:
                service_duree_connexion(client_sockfd, debut_connexion);
                break;
                
            default:
                printf("   ❌ Service invalide\n");
                fflush(stdout);
        }
    }

fin:
    close(client_sockfd);
    exit(0);
}

/*
 * Fonction: thread_serveur_func
 * ----------------------------
 * Fonction du thread serveur
 */
gpointer thread_serveur_func(gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    while (app->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sockfd = accept(app->sockfd, 
                                   (struct sockaddr *)&client_addr, 
                                   &client_len);
        
        if (client_sockfd < 0) {
            if (app->running) {
                afficher_log("❌ Erreur accept\n");
            }
            continue;
        }

        afficher_log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        afficher_log("🆕 Nouvelle connexion!\n");

        // Créer un processus fils
        pid_t pid = fork();

        if (pid < 0) {
            afficher_log("❌ Erreur fork\n");
            close(client_sockfd);
            continue;
        }

        if (pid == 0) {
            // PROCESSUS FILS
            close(app->sockfd);
            traiter_client(client_sockfd);
        } else {
            // PROCESSUS PÈRE
            close(client_sockfd);
            char log[256];
            sprintf(log, "✅ Client géré par processus %d\n", pid);
            afficher_log(log);
        }
    }
    
    return NULL;
}

/*
 * Fonction: on_demarrer_clicked
 */
void on_demarrer_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(app->entry_port));
    const char *username = gtk_entry_get_text(GTK_ENTRY(app->entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(app->entry_password));
    
    if (strlen(port_str) == 0 || strlen(username) == 0 || 
        strlen(password) == 0) {
        afficher_log("❌ Erreur: Veuillez remplir tous les champs\n\n");
        return;
    }
    
    // Sauvegarder les identifiants
    strcpy(global_username, username);
    strcpy(global_password, password);
    
    int port = atoi(port_str);
    int option = 1;
    
    // Configurer le gestionnaire de signal
    signal(SIGCHLD, gestionnaire_sigchld);
    
    // Créer le socket TCP
    app->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (app->sockfd < 0) {
        afficher_log("❌ Erreur: Impossible de créer le socket\n\n");
        return;
    }
    
    // Option SO_REUSEADDR
    setsockopt(app->sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    // Configurer l'adresse
    struct sockaddr_in serveur_addr;
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    serveur_addr.sin_port = htons(port);
    
    // Lier le socket
    if (bind(app->sockfd, (struct sockaddr *)&serveur_addr, 
             sizeof(serveur_addr)) < 0) {
        afficher_log("❌ Erreur: Impossible de lier le socket\n\n");
        close(app->sockfd);
        return;
    }
    
    // Mettre en écoute
    listen(app->sockfd, 5);
    
    // Afficher les infos
    char message[512];
    sprintf(message, "╔════════════════════════════════════════╗\n");
    afficher_log(message);
    sprintf(message, "║   SERVEUR TCP MULTISERVICE DÉMARRÉ    ║\n");
    afficher_log(message);
    sprintf(message, "╚════════════════════════════════════════╝\n");
    afficher_log(message);
    sprintf(message, "🔌 Port: %d\n", port);
    afficher_log(message);
    sprintf(message, "👤 Compte: %s / %s\n", username, password);
    afficher_log(message);
    afficher_log("📋 Services disponibles:\n");
    afficher_log("   [1] Date et Heure\n");
    afficher_log("   [2] Liste des fichiers\n");
    afficher_log("   [3] Contenu d'un fichier\n");
    afficher_log("   [4] Durée de connexion\n");
    afficher_log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    afficher_log("👂 En attente de clients...\n\n");
    
    // Activer le serveur
    app->running = TRUE;
    gtk_widget_set_sensitive(app->btn_demarrer, FALSE);
    gtk_widget_set_sensitive(app->btn_arreter, TRUE);
    gtk_widget_set_sensitive(app->entry_port, FALSE);
    gtk_widget_set_sensitive(app->entry_username, FALSE);
    gtk_widget_set_sensitive(app->entry_password, FALSE);
    
    // Démarrer le thread serveur
    app->thread_serveur = g_thread_new("serveur", thread_serveur_func, app);
}

/*
 * Fonction: on_arreter_clicked
 */
void on_arreter_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    app->running = FALSE;
    
    // Fermer le socket pour débloquer accept()
    if (app->sockfd >= 0) {
        shutdown(app->sockfd, SHUT_RDWR);
        close(app->sockfd);
    }
    
    // Attendre que le thread se termine
    if (app->thread_serveur) {
        g_thread_join(app->thread_serveur);
        app->thread_serveur = NULL;
    }
    
    afficher_log("\n🛑 Serveur arrêté\n\n");
    
    gtk_widget_set_sensitive(app->btn_demarrer, TRUE);
    gtk_widget_set_sensitive(app->btn_arreter, FALSE);
    gtk_widget_set_sensitive(app->entry_port, TRUE);
    gtk_widget_set_sensitive(app->entry_username, TRUE);
    gtk_widget_set_sensitive(app->entry_password, TRUE);
}

/*
 * Fonction: on_window_destroy
 */
void on_window_destroy(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    
    if (app->running) {
        app->running = FALSE;
        if (app->sockfd >= 0) {
            shutdown(app->sockfd, SHUT_RDWR);
            close(app->sockfd);
        }
        if (app->thread_serveur) {
            g_thread_join(app->thread_serveur);
        }
    }
    
    gtk_main_quit();
}

/*
 * Fonction: main
 */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets *app = g_malloc(sizeof(AppWidgets));
    app->running = FALSE;
    app->thread_serveur = NULL;
    global_app = app;
    
    // FENÊTRE PRINCIPALE
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), 
                        "🖥️  Serveur TCP Multiservice");
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
        "<span font='16' weight='bold'>🖥️  Serveur TCP Multiservice</span>");
    gtk_box_pack_start(GTK_BOX(vbox), label_titre, FALSE, FALSE, 5);
    
    // SÉPARATEUR
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 5);
    
    // SECTION CONFIGURATION
    GtkWidget *label_config = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_config), 
        "<b>🔧 Configuration</b>");
    gtk_widget_set_halign(label_config, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_config, FALSE, FALSE, 0);
    
    // Port
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl1 = gtk_label_new("🔌 Port:");
    gtk_widget_set_size_request(lbl1, 100, -1);
    app->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_port), "6000");
    gtk_box_pack_start(GTK_BOX(hbox1), lbl1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), app->entry_port, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    
    // Username
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl2 = gtk_label_new("👤 Username:");
    gtk_widget_set_size_request(lbl2, 100, -1);
    app->entry_username = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_username), "admin");
    gtk_box_pack_start(GTK_BOX(hbox2), lbl2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), app->entry_username, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    
    // Password
    GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl3 = gtk_label_new("🔒 Password:");
    gtk_widget_set_size_request(lbl3, 100, -1);
    app->entry_password = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_password), "admin123");
    gtk_box_pack_start(GTK_BOX(hbox3), lbl3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), app->entry_password, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 0);
    
    // BOUTONS
    GtkWidget *hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    app->btn_demarrer = gtk_button_new_with_label("▶️  Démarrer");
    app->btn_arreter = gtk_button_new_with_label("⏹️  Arrêter");
    gtk_widget_set_sensitive(app->btn_arreter, FALSE);
    g_signal_connect(app->btn_demarrer, "clicked", 
                     G_CALLBACK(on_demarrer_clicked), app);
    g_signal_connect(app->btn_arreter, "clicked", 
                     G_CALLBACK(on_arreter_clicked), app);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), app->btn_demarrer, 
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), app->btn_arreter, 
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);
    
    // SÉPARATEUR
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 5);
    
    // ZONE DE TEXTE
    GtkWidget *label_logs = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_logs), 
        "<b>📋 Logs du serveur</b>");
    gtk_widget_set_halign(label_logs, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_logs, FALSE, FALSE, 0);
    
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

#include <adwaita.h>
#include "database.h"
#include "contact_object.h"

// A global pointer to the database instance
static Database* db;
// The data store for our list view
static GListStore* store;
// The selection model to track selected contact
static GtkSingleSelection* selection;
// The sort model for the list view
static GtkSortListModel* sort_model;

// Labels for displaying selected contact details
static GtkWidget* detail_name_label;
static GtkWidget* detail_phone_label;
static GtkWidget* detail_email_label;

// --- Forward Declarations ---
static void on_add_clicked(GtkButton* button, gpointer window);
static void on_edit_clicked(GtkButton* button, gpointer window);
static void on_del_clicked(GtkButton* button, gpointer window);
static void on_search_changed(GtkSearchEntry* entry, GtkCustomFilter* filter);
static gboolean filter_func(gpointer item, gpointer user_data);
static void setup_list_item(GtkListItemFactory* factory, GtkListItem* list_item);
static void bind_list_item(GtkListItemFactory* factory, GtkListItem* list_item);
static void populate_store();
static void show_contact_dialog(GtkWindow* parent, Contact* contact_to_edit);
static void on_sort_selected(GtkDropDown* dropdown, GParamSpec* pspec);
static void on_selection_changed(GtkSingleSelection* selection, GParamSpec* pspec);
static void update_detail_view(Contact* contact);
static void on_import_file_chosen(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_export_file_chosen(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_import_clicked(GtkButton* button, gpointer window);
static void on_export_clicked(GtkButton* button, gpointer window);
static void on_about_clicked(GtkButton* button, gpointer window);
static void on_clear_search_clicked(GtkButton* button, GtkSearchEntry* search_entry);

static void on_clear_search_clicked(GtkButton* button, GtkSearchEntry* search_entry) {
    gtk_editable_set_text(GTK_EDITABLE(search_entry), "");
}

// --- Main Application Activation ---
static void on_app_activate(GApplication* app) {
    // Create the main window
    GtkWidget* window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Contact Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);

    // Create a header bar
    GtkHeaderBar* header = GTK_HEADER_BAR(gtk_header_bar_new());
    gtk_window_set_titlebar(GTK_WINDOW(window), GTK_WIDGET(header));

    // Add button
    GtkWidget* add_button = gtk_button_new_from_icon_name("list-add-symbolic");
    gtk_header_bar_pack_start(header, add_button);
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_clicked), window);

    // Edit button
    GtkWidget* edit_button = gtk_button_new_from_icon_name("document-edit-symbolic");
    gtk_header_bar_pack_start(header, edit_button);
    g_signal_connect(edit_button, "clicked", G_CALLBACK(on_edit_clicked), window);

    // Delete button
    GtkWidget* del_button = gtk_button_new_from_icon_name("edit-delete-symbolic");
    gtk_header_bar_pack_start(header, del_button);
    g_signal_connect(del_button, "clicked", G_CALLBACK(on_del_clicked), window);

    // Import button
    GtkWidget* import_button = gtk_button_new_from_icon_name("document-open-symbolic");
    gtk_header_bar_pack_end(header, import_button);
    g_signal_connect(import_button, "clicked", G_CALLBACK(on_import_clicked), window);

    // Export button
    GtkWidget* export_button = gtk_button_new_from_icon_name("document-save-symbolic");
    gtk_header_bar_pack_end(header, export_button);
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_clicked), window);

    // Sort dropdown
    GtkStringList* sort_options = gtk_string_list_new(NULL);
    gtk_string_list_append(sort_options, "Name (A-Z)");
    gtk_string_list_append(sort_options, "Name (Z-A)");
    gtk_string_list_append(sort_options, "Phone (A-Z)");
    gtk_string_list_append(sort_options, "Phone (Z-A)");
    gtk_string_list_append(sort_options, "Email (A-Z)");
    gtk_string_list_append(sort_options, "Email (Z-A)");

    GtkWidget* sort_dropdown = gtk_drop_down_new(G_LIST_MODEL(sort_options), NULL);
    gtk_header_bar_pack_end(header, sort_dropdown);
    g_signal_connect(sort_dropdown, "notify::selected", G_CALLBACK(on_sort_selected), NULL);

    // About button
    GtkWidget* about_button = gtk_button_new_from_icon_name("help-about-symbolic");
    gtk_header_bar_pack_end(header, about_button);
    g_signal_connect(about_button, "clicked", G_CALLBACK(on_about_clicked), window);

    // Main vertical box
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // --- Search and Filter Setup ---
    GtkCustomFilter* filter = gtk_custom_filter_new(filter_func, NULL, NULL);
    GtkFilterListModel* filter_model = gtk_filter_list_model_new(G_LIST_MODEL(store), GTK_FILTER(filter));
    GtkSorter* sorter = GTK_SORTER(gtk_custom_sorter_new(contact_object_compare, NULL, NULL));
    sort_model = gtk_sort_list_model_new(G_LIST_MODEL(filter_model), sorter);
    selection = gtk_single_selection_new(G_LIST_MODEL(sort_model));

    // Search Entry and Clear Button
    GtkWidget* search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(search_box, 6);
    gtk_widget_set_margin_end(search_box, 6);
    gtk_box_append(GTK_BOX(vbox), search_box);

    GtkWidget* search_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_box_append(GTK_BOX(search_box), search_entry);

    GtkWidget* clear_search_button = gtk_button_new_from_icon_name("edit-clear-symbolic");
    gtk_button_set_has_frame(GTK_BUTTON(clear_search_button), FALSE);
    gtk_box_append(GTK_BOX(search_box), clear_search_button);
    g_signal_connect(clear_search_button, "clicked", G_CALLBACK(on_clear_search_clicked), search_entry);

    // --- Contact Details View ---
    GtkWidget* details_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_set_margin_start(details_box, 6);
    gtk_widget_set_margin_end(details_box, 6);
    gtk_widget_set_margin_top(details_box, 6);
    gtk_widget_set_margin_bottom(details_box, 6);
    gtk_box_append(GTK_BOX(vbox), details_box);

    detail_name_label = gtk_label_new("Name: ");
    gtk_widget_set_halign(detail_name_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(details_box), detail_name_label);

    detail_phone_label = gtk_label_new("Phone: ");
    gtk_widget_set_halign(detail_phone_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(details_box), detail_phone_label);

    detail_email_label = gtk_label_new("Email: ");
    gtk_widget_set_halign(detail_email_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(details_box), detail_email_label);

    // Connect selection change signal
    g_signal_connect(selection, "notify::selected-item", G_CALLBACK(on_selection_changed), NULL);
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), filter);

    // --- List View Setup ---
    GtkWidget* scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_append(GTK_BOX(vbox), scrolled_window);

    GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_list_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_list_item), NULL);

    GtkWidget* list_view = gtk_list_view_new(GTK_SELECTION_MODEL(selection), factory);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), list_view);

    // Populate the store with initial data
    populate_store();

    gtk_widget_set_visible(window, TRUE);
}

// --- Main Function ---
int main(int argc, char* argv[]) {
    db = database_new("contact_manager_gtk.db");
    store = g_list_store_new(CONTACT_TYPE_OBJECT);

    AdwApplication* app = adw_application_new("com.example.contactmanager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(store);
    database_close(db);
    return status;
}

// --- UI Callbacks and Helpers ---

static void populate_store() {
    g_list_store_remove_all(store);
    int count;
    Contact** contacts = database_list_contacts(db, &count);
    for (int i = 0; i < count; i++) {
        g_list_store_append(store, contact_object_new(contacts[i]));
    }
}

static void setup_list_item(GtkListItemFactory* factory, GtkListItem* list_item) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);

    GtkWidget* name_label = gtk_label_new("");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(name_label), attrs);
    pango_attr_list_unref(attrs);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), name_label);

    GtkWidget* phone_label = gtk_label_new("");
    gtk_widget_set_halign(phone_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), phone_label);

    GtkWidget* email_label = gtk_label_new("");
    gtk_widget_set_halign(email_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), email_label);

    gtk_list_item_set_child(list_item, box);
}

static void bind_list_item(GtkListItemFactory* factory, GtkListItem* list_item) {
    GtkWidget* box = gtk_list_item_get_child(list_item);
    ContactObject* contact_obj = gtk_list_item_get_item(list_item);
    Contact* contact = contact_object_get_contact(contact_obj);

    GtkLabel* name_label = GTK_LABEL(gtk_widget_get_first_child(box));
    GtkLabel* phone_label = GTK_LABEL(gtk_widget_get_next_sibling(GTK_WIDGET(name_label)));
    GtkLabel* email_label = GTK_LABEL(gtk_widget_get_next_sibling(GTK_WIDGET(phone_label)));

    gtk_label_set_text(name_label, contact->name);
    gtk_label_set_text(phone_label, contact->phone);
    gtk_label_set_text(email_label, contact->email);
}

// --- Dialog and Button Logic ---

static void on_add_clicked(GtkButton* button, gpointer window) {
    show_contact_dialog(GTK_WINDOW(window), NULL);
}

static void on_edit_clicked(GtkButton* button, gpointer window) {
    guint pos = gtk_single_selection_get_selected(selection);
    if (pos != GTK_INVALID_LIST_POSITION) {
        ContactObject* contact_obj = g_list_model_get_item(G_LIST_MODEL(selection), pos);
        if (contact_obj) {
            show_contact_dialog(GTK_WINDOW(window), contact_object_get_contact(contact_obj));
            g_object_unref(contact_obj);
        }
    }
}

static void on_delete_response_cb(GObject* source, GAsyncResult* res, gpointer user_data) {
    const char* response = adw_message_dialog_choose_finish(ADW_MESSAGE_DIALOG(source), res);
    Contact* contact = user_data;

    if (response != NULL && strcmp(response, "delete") == 0) {
        database_del_contact(db, contact->name);
        database_save(db);
        populate_store();
    }
    // Free the contact data that was strdup'd for the dialog
    free(contact->name);
    free(contact->phone);
    free(contact->email);
    free(contact);
}

static void on_del_clicked(GtkButton* button, gpointer window) {
    guint pos = gtk_single_selection_get_selected(selection);
    if (pos != GTK_INVALID_LIST_POSITION) {
        ContactObject* contact_obj = g_list_model_get_item(G_LIST_MODEL(selection), pos);
        if (contact_obj) {
            Contact* contact_to_delete = contact_object_get_contact(contact_obj);

            // Create a copy of the contact to pass to the async callback
            Contact* contact_copy = malloc(sizeof(Contact));
            contact_copy->name = strdup(contact_to_delete->name);
            contact_copy->phone = strdup(contact_to_delete->phone);
            contact_copy->email = strdup(contact_to_delete->email);

            AdwMessageDialog* dialog = ADW_MESSAGE_DIALOG(adw_message_dialog_new(GTK_WINDOW(window),
                                                              "Confirm Deletion",
                                                              g_strdup_printf("Are you sure you want to delete contact '%s'?", contact_to_delete->name)));
            adw_message_dialog_add_response(dialog, "cancel", "_Cancel");
            adw_message_dialog_add_response(dialog, "delete", "_Delete");
            adw_message_dialog_set_default_response(dialog, "cancel");
            adw_message_dialog_set_close_response(dialog, "cancel");

            adw_message_dialog_choose(dialog, NULL, (GAsyncReadyCallback)on_delete_response_cb, contact_copy);
            g_object_unref(dialog);
            g_object_unref(contact_obj);
        }
    }
}

// --- Modern AdwMessageDialog for Add/Edit ---

typedef struct {
    GtkEntry* name_entry;
    GtkEntry* phone_entry;
    GtkEntry* email_entry;
    Contact* original_contact;
} DialogWidgets;

static void dialog_response_cb(GObject* source, GAsyncResult* res, gpointer user_data) {
    DialogWidgets* widgets = user_data;
    const char* response = adw_message_dialog_choose_finish(ADW_MESSAGE_DIALOG(source), res);

    if (response != NULL && strcmp(response, "save") == 0) {
        const char* name = gtk_editable_get_text(GTK_EDITABLE(widgets->name_entry));
        const char* phone = gtk_editable_get_text(GTK_EDITABLE(widgets->phone_entry));
        const char* email = gtk_editable_get_text(GTK_EDITABLE(widgets->email_entry));

        if (strlen(name) == 0) {
            AdwMessageDialog* error_dialog = ADW_MESSAGE_DIALOG(adw_message_dialog_new(GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(source))),
                                                                    "Error", 
                                                                    "Contact name cannot be empty."));
            adw_message_dialog_add_response(error_dialog, "ok", "_Ok");
            adw_message_dialog_set_default_response(error_dialog, "ok");
            adw_message_dialog_set_close_response(error_dialog, "ok");
            adw_message_dialog_choose(error_dialog, NULL, NULL, NULL);
            g_object_unref(error_dialog);
            // Do not free widgets here, it will be freed at the end of the function
            return;
        }

        if (widgets->original_contact) { // Editing existing contact
            free(widgets->original_contact->name);
            free(widgets->original_contact->phone);
            free(widgets->original_contact->email);
            widgets->original_contact->name = strdup(name);
            widgets->original_contact->phone = strdup(phone);
            widgets->original_contact->email = strdup(email);
        } else { // Adding new contact
            Contact* new_contact = malloc(sizeof(Contact));
            new_contact->name = strdup(name);
            new_contact->phone = strdup(phone);
            new_contact->email = strdup(email);
            database_add_contact(db, new_contact);
        }
        database_save(db);
        populate_store();
    }
    g_slice_free(DialogWidgets, widgets);
}

static void show_contact_dialog(GtkWindow* parent, Contact* contact_to_edit) {
    const char* title = contact_to_edit ? "Edit Contact" : "Add Contact";

    GtkWidget* content_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(content_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(content_grid), 10);

    DialogWidgets* widgets = g_slice_new(DialogWidgets);
    widgets->original_contact = contact_to_edit;
    widgets->name_entry = GTK_ENTRY(gtk_entry_new());
    widgets->phone_entry = GTK_ENTRY(gtk_entry_new());
    widgets->email_entry = GTK_ENTRY(gtk_entry_new());

    if (contact_to_edit) {
        gtk_editable_set_text(GTK_EDITABLE(widgets->name_entry), contact_to_edit->name);
        gtk_editable_set_text(GTK_EDITABLE(widgets->phone_entry), contact_to_edit->phone);
        gtk_editable_set_text(GTK_EDITABLE(widgets->email_entry), contact_to_edit->email);
    }

    gtk_grid_attach(GTK_GRID(content_grid), gtk_label_new("Name:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(content_grid), GTK_WIDGET(widgets->name_entry), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(content_grid), gtk_label_new("Phone:"), 0, 1, 1, 1); 
    gtk_grid_attach(GTK_GRID(content_grid), GTK_WIDGET(widgets->phone_entry), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(content_grid), gtk_label_new("Email:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(content_grid), GTK_WIDGET(widgets->email_entry), 1, 2, 1, 1);
 
    AdwMessageDialog* dialog = ADW_MESSAGE_DIALOG(adw_message_dialog_new(GTK_WINDOW(parent), title, NULL));
    adw_message_dialog_set_extra_child(dialog, content_grid);
    adw_message_dialog_add_response(dialog, "cancel", "_Cancel");
    adw_message_dialog_add_response(dialog, "save", "_Save");
    adw_message_dialog_set_default_response(dialog, "save");
    adw_message_dialog_set_close_response(dialog, "cancel");

    adw_message_dialog_choose(dialog, NULL, (GAsyncReadyCallback)dialog_response_cb, widgets);
    g_object_unref(dialog);
}


// --- Search and Filter Logic ---
static gchar* search_text = NULL;

static gboolean filter_func(gpointer item, gpointer user_data) {
    ContactObject* contact_obj = (ContactObject*)item;
    Contact* contact = contact_object_get_contact(contact_obj);

    if (search_text == NULL || *search_text == '\0') {
        return TRUE;
    }
    return (g_strrstr_len(contact->name, -1, search_text) != NULL);
}

static void on_search_changed(GtkSearchEntry* entry, GtkCustomFilter* filter) {
    g_free(search_text);
    search_text = g_strdup(gtk_editable_get_text(GTK_EDITABLE(entry)));
    gtk_filter_changed(GTK_FILTER(filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void on_sort_selected(GtkDropDown* dropdown, GParamSpec* pspec) {
    guint selected = gtk_drop_down_get_selected(dropdown);
    contact_object_set_sort_order((ContactSortOrder)selected);
    gtk_sort_list_model_set_sorter(sort_model, GTK_SORTER(gtk_custom_sorter_new(contact_object_compare, NULL, NULL)));
}

static void on_selection_changed(GtkSingleSelection* selection, GParamSpec* pspec) {
    ContactObject* contact_obj = NULL;
    Contact* contact = NULL;

    guint pos = gtk_single_selection_get_selected(selection);
    if (pos != GTK_INVALID_LIST_POSITION) {
        contact_obj = g_list_model_get_item(G_LIST_MODEL(selection), pos);
        if (contact_obj) {
            contact = contact_object_get_contact(contact_obj);
        }
    }
    update_detail_view(contact);
    if (contact_obj) {
        g_object_unref(contact_obj);
    }
}

static void update_detail_view(Contact* contact) {
    if (contact) {
        gtk_label_set_text(GTK_LABEL(detail_name_label), g_strdup_printf("<b>Name:</b> %s", contact->name));
        gtk_label_set_markup(GTK_LABEL(detail_name_label), g_strdup_printf("<b>Name:</b> %s", contact->name));
        gtk_label_set_text(GTK_LABEL(detail_phone_label), g_strdup_printf("<b>Phone:</b> %s", contact->phone));
        gtk_label_set_markup(GTK_LABEL(detail_phone_label), g_strdup_printf("<b>Phone:</b> %s", contact->phone));
        gtk_label_set_text(GTK_LABEL(detail_email_label), g_strdup_printf("<b>Email:</b> %s", contact->email));
        gtk_label_set_markup(GTK_LABEL(detail_email_label), g_strdup_printf("<b>Email:</b> %s", contact->email));
    } else {
        gtk_label_set_text(GTK_LABEL(detail_name_label), "<b>Name:</b>");
        gtk_label_set_markup(GTK_LABEL(detail_name_label), "<b>Name:</b>");
        gtk_label_set_text(GTK_LABEL(detail_phone_label), "<b>Phone:</b>");
        gtk_label_set_markup(GTK_LABEL(detail_phone_label), "<b>Phone:</b>");
        gtk_label_set_text(GTK_LABEL(detail_email_label), "<b>Email:</b>");
        gtk_label_set_markup(GTK_LABEL(detail_email_label), "<b>Email:</b>");
    }
}

static void on_import_file_chosen(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file = gtk_file_dialog_open_finish(dialog, res, NULL);
    if (file) {
        char *filepath = g_file_get_path(file);
        database_import(db, filepath);
        populate_store();
        g_free(filepath);
        g_object_unref(file);
    }
    g_object_unref(dialog);
}

static void on_export_file_chosen(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file = gtk_file_dialog_save_finish(dialog, res, NULL);
    if (file) {
        char *filepath = g_file_get_path(file);
        database_export(db, filepath);
        g_free(filepath);
        g_object_unref(file);
    }
    g_object_unref(dialog);
}

static void on_import_clicked(GtkButton* button, gpointer window) {
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Import Contacts");
    GListStore *filters_store = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "vCard files (*.vcf, *.vcard)");
    gtk_file_filter_add_pattern(filter, "*.vcf");
    gtk_file_filter_add_pattern(filter, "*.vcard");
    g_list_store_append(filters_store, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters_store));
    g_object_unref(filters_store); // The dialog takes ownership of the list model
    gtk_file_dialog_open(dialog, GTK_WINDOW(window), NULL, (GAsyncReadyCallback)on_import_file_chosen, NULL);
}

static void on_export_clicked(GtkButton* button, gpointer window) {
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Export Contacts");
    GListStore *filters_store = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "vCard files (*.vcf, *.vcard)");
    gtk_file_filter_add_pattern(filter, "*.vcf");
    gtk_file_filter_add_pattern(filter, "*.vcard");
    g_list_store_append(filters_store, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters_store));
    g_object_unref(filters_store); // The dialog takes ownership of the list model
    gtk_file_dialog_save(dialog, GTK_WINDOW(window), NULL, (GAsyncReadyCallback)on_export_file_chosen, NULL);
}

static void on_about_clicked(GtkButton* button, gpointer window) {
    AdwAboutWindow* about_window = ADW_ABOUT_WINDOW(adw_about_window_new());
    gtk_window_set_transient_for(GTK_WINDOW(about_window), GTK_WINDOW(window));
    gtk_window_set_modal(GTK_WINDOW(about_window), TRUE);

    adw_about_window_set_application_name(about_window, "Contact Manager");
    adw_about_window_set_application_icon(about_window, "contact-manager-symbolic"); // You might need to provide this icon
    adw_about_window_set_version(about_window, "1.0.0");
    adw_about_window_set_developer_name(about_window, "Your Name"); // Replace with actual developer name
    adw_about_window_set_copyright(about_window, "© 2023 Your Name"); // Replace with actual copyright
    adw_about_window_set_license_type(about_window, GTK_LICENSE_MIT_X11); // Or choose another license
    adw_about_window_set_comments(about_window, "A simple contact manager application.");
    adw_about_window_set_website(about_window, "https://example.com"); // Replace with actual website

    gtk_window_present(GTK_WINDOW(about_window));
}
#include "contact_object.h"
#include <string.h>

static ContactSortOrder current_sort_order = CONTACT_SORT_ORDER_NAME_ASC;

struct _ContactObject {
    GObject parent_instance;
    Contact* contact;
};

G_DEFINE_TYPE(ContactObject, contact_object, G_TYPE_OBJECT)

static void contact_object_finalize(GObject* gobject) {
    ContactObject* self = CONTACT_OBJECT(gobject);
    // We don't free the contact here, as it's owned by the database
    G_OBJECT_CLASS(contact_object_parent_class)->finalize(gobject);
}

static void contact_object_class_init(ContactObjectClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = contact_object_finalize;
}

static void contact_object_init(ContactObject* self) {
}

ContactObject* contact_object_new(Contact* contact) {
    ContactObject* self = g_object_new(CONTACT_TYPE_OBJECT, NULL);
    self->contact = contact;
    return self;
}

Contact* contact_object_get_contact(ContactObject* self) {
    return self->contact;
}

void contact_object_set_sort_order(ContactSortOrder order) {
    current_sort_order = order;
}

gint contact_object_compare(gconstpointer a, gconstpointer b, gpointer user_data) {
    const Contact* contact_a = contact_object_get_contact(CONTACT_OBJECT((gpointer)a));
    const Contact* contact_b = contact_object_get_contact(CONTACT_OBJECT((gpointer)b));
    gint cmp = 0;

    switch (current_sort_order) {
        case CONTACT_SORT_ORDER_NAME_ASC:
            cmp = strcmp(contact_a->name, contact_b->name);
            break;
        case CONTACT_SORT_ORDER_NAME_DESC:
            cmp = strcmp(contact_b->name, contact_a->name);
            break;
        case CONTACT_SORT_ORDER_PHONE_ASC:
            cmp = strcmp(contact_a->phone, contact_b->phone);
            break;
        case CONTACT_SORT_ORDER_PHONE_DESC:
            cmp = strcmp(contact_b->phone, contact_a->phone);
            break;
        case CONTACT_SORT_ORDER_EMAIL_ASC:
            cmp = strcmp(contact_a->email, contact_b->email);
            break;
        case CONTACT_SORT_ORDER_EMAIL_DESC:
            cmp = strcmp(contact_b->email, contact_a->email);
            break;
    }
    return cmp;
}

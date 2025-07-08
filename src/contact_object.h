#ifndef CONTACT_OBJECT_H
#define CONTACT_OBJECT_H

#include <glib-object.h>
#include "database.h"

#define CONTACT_TYPE_OBJECT (contact_object_get_type())
G_DECLARE_FINAL_TYPE(ContactObject, contact_object, CONTACT, OBJECT, GObject)

typedef enum {
    CONTACT_SORT_ORDER_NAME_ASC,
    CONTACT_SORT_ORDER_NAME_DESC,
    CONTACT_SORT_ORDER_PHONE_ASC,
    CONTACT_SORT_ORDER_PHONE_DESC,
    CONTACT_SORT_ORDER_EMAIL_ASC,
    CONTACT_SORT_ORDER_EMAIL_DESC
} ContactSortOrder;

ContactObject* contact_object_new(Contact* contact);
Contact* contact_object_get_contact(ContactObject* self);
void contact_object_set_sort_order(ContactSortOrder order);
gint contact_object_compare(gconstpointer a, gconstpointer b, gpointer user_data);

#endif // CONTACT_OBJECT_H

/* json-object.c - JSON object implementation
 * 
 * This file is part of JSON-GLib
 * Copyright (C) 2007  OpenedHand Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Author:
 *   Emmanuele Bassi  <ebassi@openedhand.com>
 */

#include "config.h"

#include <glib.h>

#include "json-types.h"
#include "json-private.h"

/**
 * SECTION:json-object
 * @short_description: a JSON object representation
 *
 * #JsonArray is the representation of the object type inside JSON. It contains
 * #JsonNode<!-- -->s, which may contain fundamental types, arrays or other
 * objects. Each member of an object is accessed using its name.
 *
 * Since objects can be expensive, they are reference counted. You can control
 * the lifetime of a #JsonObject using json_object_ref() and json_object_unref().
 *
 * To extract a member with a given name, use json_object_get_member().
 * To retrieve the list of members, use json_object_get_members().
 * To retrieve the size of the object (that is, the number of members it has), use
 * json_object_get_size().
 */

struct _JsonObject
{
  GHashTable *members;

  volatile gint ref_count;
};

JsonObject *
json_object_new (void)
{
  JsonObject *object;

  object = g_slice_new (JsonObject);
  
  object->ref_count = 1;
  object->members = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free,
                                           (GDestroyNotify) json_node_free);

  return object;
}

/**
 * json_object_ref:
 * @object: a #JsonObject
 *
 * Increase by one the reference count of a #JsonObject.
 *
 * Return value: the passed #JsonObject, with the reference count
 *   increased by one.
 */
JsonObject *
json_object_ref (JsonObject *object)
{
  g_return_val_if_fail (object != NULL, NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);

  g_atomic_int_exchange_and_add (&object->ref_count, 1);

  return object;
}

/**
 * json_object_unref:
 * @object: a #JsonObject
 *
 * Decreases by one the reference count of a #JsonObject. If the
 * reference count reaches zero, the object is destroyed and all
 * its allocated resources are freed.
 */
void
json_object_unref (JsonObject *object)
{
  gint old_ref;

  g_return_if_fail (object != NULL);
  g_return_if_fail (object->ref_count > 0);

  old_ref = g_atomic_int_get (&object->ref_count);
  if (old_ref > 1)
    g_atomic_int_compare_and_exchange (&object->ref_count, old_ref, old_ref - 1);
  else
    {
      g_hash_table_destroy (object->members);
      object->members = NULL;

      g_slice_free (JsonObject, object);
    }
}

void
json_object_add_member (JsonObject  *object,
                        const gchar *member_name,
                        JsonNode    *node)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (member_name != NULL);
  g_return_if_fail (node != NULL);

  if (json_object_has_member (object, member_name))
    {
      g_warning ("JsonObject already has a `%s' member of type `%s'",
                 member_name,
                 json_node_type_name (node));
      return;
    }

  g_hash_table_replace (object->members, g_strdup (member_name), node);
}

/**
 * json_object_get_members:
 * @object: a #JsonObject
 *
 * Retrieves all the names of the members of a #JsonObject. You can
 * obtain the value for each member using json_object_get_member().
 *
 * Return value: a #GList of member names. The content of the list
 *   is owned by the #JsonObject and should never be modified or
 *   freed. When you have finished using the returned list, use
 *   g_slist_free() to free the resources it has allocated.
 */
GList *
json_object_get_members (JsonObject *object)
{
  g_return_val_if_fail (object != NULL, NULL);

  return g_hash_table_get_keys (object->members);
}

/**
 * json_object_get_member:
 * @object: a #JsonObject
 * @member_name: the name of the JSON object member to access
 *
 * Retrieves the value of @member_name inside a #JsonObject.
 *
 * Return value: a pointer to the value for the requested object
 *   member, or %NULL
 */
JsonNode *
json_object_get_member (JsonObject *object,
                        const gchar *member_name)
{
  g_return_val_if_fail (object != NULL, NULL);
  g_return_val_if_fail (member_name != NULL, NULL);

  return g_hash_table_lookup (object->members, member_name);
}

/**
 * json_object_has_member:
 * @object: a #JsonObject
 * @member_name: the name of a JSON object member
 *
 * Checks whether @object has a member named @member_name.
 *
 * Return value: %TRUE if the JSON object has the requested member
 */
gboolean
json_object_has_member (JsonObject *object,
                        const gchar *member_name)
{
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (member_name != NULL, FALSE);
  
  return (g_hash_table_lookup (object->members, member_name) != NULL);
}

/**
 * json_object_get_size:
 * @object: a #JsonObject
 *
 * Retrieves the size of a #JsonObject.
 *
 * Return value: the number of members the JSON object has
 */
guint
json_object_get_size (JsonObject *object)
{
  g_return_val_if_fail (object != NULL, 0);

  return g_hash_table_size (object->members);
}

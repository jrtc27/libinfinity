/* infinote - Collaborative notetaking application
 * Copyright (C) 2007 Armin Burgmeier
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libinfgtk/inf-gtk-browser-model.h>
#include <libinfinity/client/infc-browser.h>

#include <gtk/gtktreemodel.h>

/* TODO: Emit has-child-toggled signals */

typedef struct _InfGtkBrowserModelItem InfGtkBrowserModelItem;
struct _InfGtkBrowserModelItem {
  InfDiscovery* discovery;
  InfDiscoveryInfo* info;
  InfcBrowser* browser;
  InfGtkBrowserModelStatus status;
  GError* error;
  InfGtkBrowserModelItem* next;
};

typedef struct _InfGtkBrowserModelPrivate InfGtkBrowserModelPrivate;
struct _InfGtkBrowserModelPrivate {
  gint stamp;
  InfConnectionManager* connection_manager;

  GSList* discoveries;
  InfGtkBrowserModelItem* first_item;
  InfGtkBrowserModelItem* last_item;
};

enum {
  PROP_0,

  PROP_CONNECTION_MANAGER
};

#define INF_GTK_BROWSER_MODEL_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), INF_GTK_TYPE_BROWSER_MODEL, InfGtkBrowserModelPrivate))

static GObjectClass* parent_class;

/*
 * Utility functions
 */

static InfGtkBrowserModelItem*
inf_gtk_browser_model_find_item_by_connection(InfGtkBrowserModel* model,
                                              InfXmlConnection* connection)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  for(item = priv->first_item; item != NULL; item = item->next)
    if(item->browser != NULL)
      if(infc_browser_get_connection(item->browser) == connection)
        return item;

  return NULL;
}

static InfGtkBrowserModelItem*
inf_gtk_browser_model_find_item_by_browser(InfGtkBrowserModel* model,
                                           InfcBrowser* browser)
{
  InfXmlConnection* connection;
  connection = infc_browser_get_connection(browser);
  return inf_gtk_browser_model_find_item_by_connection(model, connection);
}

static InfGtkBrowserModelItem*
inf_gtk_browser_model_find_item_by_discovery_info(InfGtkBrowserModel* model,
                                                  InfDiscoveryInfo* info)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  for(item = priv->first_item; item != NULL; item = item->next)
    if(item->info != NULL)
      if(item->info == info)
        return item;

  return NULL;
}

/*
 * Callbacks and signal handlers
 */

/* Required by inf_gtk_browser_model_resolv_complete_func */
static void
inf_gtk_browser_model_item_set_connection(InfGtkBrowserModel* model,
                                          InfGtkBrowserModelItem* item,
                                          InfXmlConnection* connection);

/* Required by inf_gtk_browser_model_discovered_cb */
static InfGtkBrowserModelItem*
inf_gtk_browser_model_add_item(InfGtkBrowserModel* model,
                               InfDiscovery* discovery,
                               InfDiscoveryInfo* info,
                               InfXmlConnection* connection);

/* Required by inf_gtk_browser_model_status_notify_cb */
static void
inf_gtk_browser_model_remove_item(InfGtkBrowserModel* model,
                                  InfGtkBrowserModelItem* item);

static void
inf_gtk_browser_model_discovered_cb(InfDiscovery* discovery,
                                    InfDiscoveryInfo* info,
                                    gpointer user_data)
{
  inf_gtk_browser_model_add_item(
    INF_GTK_BROWSER_MODEL(user_data),
    discovery,
    info,
    NULL
  );
}

static void
inf_gtk_browser_model_undiscovered_cb(InfDiscovery* discovery,
                                      InfDiscoveryInfo* info,
                                      gpointer user_data)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelItem* item;

  model = INF_GTK_BROWSER_MODEL(user_data);
  item = inf_gtk_browser_model_find_item_by_discovery_info(model, info);
  g_assert(item != NULL);

  /* TODO: Keep if connection exists, just reset discovery and info */
  inf_gtk_browser_model_remove_item(model, item);
}

static void
inf_gtk_browser_model_status_notify_cb(GObject* object,
                                       GParamSpec* pspec,
                                       gpointer user_data)
{
  InfGtkBrowserModel* model;
  InfXmlConnection* connection;
  InfGtkBrowserModelItem* item;
  InfXmlConnectionStatus status;

  model = INF_GTK_BROWSER_MODEL(user_data);
  connection = INF_XML_CONNECTION(object);
  g_object_get(G_OBJECT(connection), "status", &status, NULL);

  if(status == INF_XML_CONNECTION_CLOSED ||
     status == INF_XML_CONNECTION_CLOSING)
  {
    item = inf_gtk_browser_model_find_item_by_connection(model, connection);
    g_assert(item != NULL);

    /* TODO: If disocvery info is still present, keep entry,
     * just reset browser and show a "disconnected" error. */
    inf_gtk_browser_model_remove_item(model, item);
  }
}

static void
inf_gtk_browser_model_add_node_cb(InfcBrowser* browser,
                                  InfcBrowserIter* iter,
                                  gpointer user_data)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  GtkTreeIter tree_iter;
  GtkTreePath* path;

  model = INF_GTK_BROWSER_MODEL(user_data);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  item = inf_gtk_browser_model_find_item_by_browser(model, browser);

  tree_iter.stamp = priv->stamp;
  tree_iter.user_data = item;
  tree_iter.user_data2 = GUINT_TO_POINTER(iter->node_id);
  tree_iter.user_data3 = iter->node;

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &tree_iter);
  gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), path, &tree_iter);
  gtk_tree_path_free(path);
}

static void
inf_gtk_browser_model_remove_node_cb(InfcBrowser* browser,
                                     InfcBrowserIter* iter,
                                     gpointer user_data)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  GtkTreeIter tree_iter;
  GtkTreePath* path;

  model = INF_GTK_BROWSER_MODEL(user_data);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  item = inf_gtk_browser_model_find_item_by_browser(model, browser);

  tree_iter.stamp = priv->stamp;
  tree_iter.user_data = item;
  tree_iter.user_data2 = GUINT_TO_POINTER(iter->node_id);
  tree_iter.user_data3 = iter->node;

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &tree_iter);
  /* TODO: The GTK+ docs say we should call this when the row has actually
   * been removed from the model, but InfcBrowser does this _after_ having
   * emitted the signal. Probably, it should do it in the default signal
   * handler, and invalidate the iterator there after the row was removed. */
  gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), path);
  gtk_tree_path_free(path);
}

static void
inf_gtk_browser_model_resolv_complete_func(InfDiscoveryInfo* info,
                                           InfXmlConnection* connection,
                                           gpointer user_data)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* new_item;
  InfGtkBrowserModelItem* old_item;
  GtkTreeIter tree_iter;
  GtkTreePath* path;

  InfGtkBrowserModelItem* cur;
  gint* order;
  guint count;
  guint new_pos;
  guint old_pos;
  guint i;

  model = INF_GTK_BROWSER_MODEL(user_data);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  new_item = inf_gtk_browser_model_find_item_by_discovery_info(model, info);
  old_item = inf_gtk_browser_model_find_item_by_connection(model, connection);
  
  g_assert(new_item != NULL);
  g_assert(new_item->status == INF_GTK_BROWSER_MODEL_RESOLVING);

  tree_iter.stamp = priv->stamp;
  tree_iter.user_data = new_item;
  tree_iter.user_data2 = GUINT_TO_POINTER(0);
  tree_iter.user_data3 = NULL;
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &tree_iter);

  if(old_item != NULL)
  {
    g_assert(old_item != new_item);

    /* There is already an item with the same connection. This is perhaps from
     * another discovery or was inserted directly. We remove the current item
     * and move the existing one to the place of it. */

    count = 0;
    for(cur = priv->first_item; cur != NULL; cur = cur->next)
    {
      if(cur == old_item) old_pos = count;
      if(cur == new_item) new_pos = count;
      ++ count;
    }

    order = g_malloc(sizeof(gint) * count);
    for(i = 0; i < count; ++ i)
      order[i] = i;

    order[old_pos] = new_pos;
    order[new_pos] = old_pos;

    gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), path);
    gtk_tree_path_free(path);
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(GTK_TREE_MODEL(model), path, NULL, order);
  }
  else
  {
    inf_gtk_browser_model_item_set_connection(model, new_item, connection);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(model), path, &tree_iter);
  }

  gtk_tree_path_free(path);
}

static void
inf_gtk_browser_model_resolv_error_func(InfDiscoveryInfo* info,
                                        const GError* error,
                                        gpointer user_data)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  InfGtkBrowserModelItem* item;
  GtkTreeIter tree_iter;
  GtkTreePath* path;

  model = INF_GTK_BROWSER_MODEL(user_data);
  item = inf_gtk_browser_model_find_item_by_discovery_info(model, info);

  g_assert(item != NULL);
  g_assert(item->status == INF_GTK_BROWSER_MODEL_RESOLVING);
  item->status = INF_GTK_BROWSER_MODEL_ERROR;
  item->error = g_error_copy(error);

  tree_iter.stamp = priv->stamp;
  tree_iter.user_data = item;
  tree_iter.user_data2 = GUINT_TO_POINTER(0);
  tree_iter.user_data3 = NULL;

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &tree_iter);
  gtk_tree_model_row_changed(GTK_TREE_MODEL(model), path, &tree_iter);
  gtk_tree_path_free(path);
}

/*
 * InfGtkBrowserModelItem construction/destruction
 */

/* Note this function does not call gtk_tree_model_row_changed */
static void
inf_gtk_browser_model_item_set_connection(InfGtkBrowserModel* model,
                                          InfGtkBrowserModelItem* item,
                                          InfXmlConnection* connection)
{
  InfGtkBrowserModelPrivate* priv;
  InfXmlConnectionStatus status;
  
  g_assert(item->browser == NULL);

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_object_get(G_OBJECT(connection), "status", &status, NULL);

  item->browser = infc_browser_new(priv->connection_manager, connection);

  switch(status)
  {
  case INF_XML_CONNECTION_OPENING:
    item->status = INF_GTK_BROWSER_MODEL_CONNECTING;
    break;
  case INF_XML_CONNECTION_OPEN:
    item->status = INF_GTK_BROWSER_MODEL_CONNECTED;
    break;
  case INF_XML_CONNECTION_CLOSING:
  case INF_XML_CONNECTION_CLOSED:
  default:
    g_assert_not_reached();
    break;
  }

  g_signal_connect(
    G_OBJECT(connection),
    "notify::status",
    G_CALLBACK(inf_gtk_browser_model_status_notify_cb),
    model
  );

  g_signal_connect_after(
    G_OBJECT(item->browser),
    "add-node",
    G_CALLBACK(inf_gtk_browser_model_add_node_cb),
    model
  );

  g_signal_connect_after(
    G_OBJECT(item->browser),
    "remove-node",
    G_CALLBACK(inf_gtk_browser_model_remove_node_cb),
    model
  );
}

static InfGtkBrowserModelItem*
inf_gtk_browser_model_add_item(InfGtkBrowserModel* model,
                               InfDiscovery* discovery,
                               InfDiscoveryInfo* info,
                               InfXmlConnection* connection)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfGtkBrowserModelItem* cur;
  GtkTreePath* path;
  GtkTreeIter iter;
  guint index;

  g_assert(
    connection == NULL ||
    inf_gtk_browser_model_find_item_by_connection(model, connection) == NULL
  );

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  item = g_slice_new(InfGtkBrowserModelItem);
  item->discovery = discovery;
  item->status = INF_GTK_BROWSER_MODEL_DISCOVERED;
  item->browser = NULL;
  item->info = info;
  item->error = NULL;
  item->next = NULL;

  if(connection != NULL)
    inf_gtk_browser_model_item_set_connection(model, item, connection);
  
  index = 0;
  for(cur = priv->first_item; cur != NULL; cur = cur->next)
    ++ index;

  /* Link */
  if(priv->first_item == NULL)
  {
    priv->first_item = item;
    priv->last_item = item;
  }
  else
  {
    priv->last_item->next = item;
    priv->last_item = item;
  }

  path = gtk_tree_path_new_from_indices(index, -1);
  iter.stamp = priv->stamp;
  iter.user_data = item;
  iter.user_data2 = GUINT_TO_POINTER(0);
  iter.user_data3 = NULL;

  gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), path, &iter);
  gtk_tree_path_free(path);

  return item;
}

static void
inf_gtk_browser_model_remove_item(InfGtkBrowserModel* model,
                                  InfGtkBrowserModelItem* item)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* prev;
  InfGtkBrowserModelItem* cur;
  GtkTreePath* path;
  InfXmlConnection* connection;
  guint index;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  /* Unlink */
  prev = NULL;
  index = 0;

  for(cur = priv->first_item; cur != NULL; cur = cur->next)
  {
    if(cur == item)
    {
      if(prev == NULL)
        priv->first_item = item->next;
      else
        prev->next = item->next;

      if(item->next == NULL)
        priv->last_item = prev;

      break;
    }
    else
    {
      prev = cur;
      ++ index;
    }
  }

  g_assert(cur != NULL);

  path = gtk_tree_path_new_from_indices(index, -1);
  gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), path);
  gtk_tree_path_free(path);
  
  if(item->error != NULL)
    g_error_free(item->error);

  if(item->browser != NULL)
  {
    connection = infc_browser_get_connection(item->browser);

    g_signal_handlers_disconnect_by_func(
      G_OBJECT(connection),
      G_CALLBACK(inf_gtk_browser_model_status_notify_cb),
      model
    );
    
    g_signal_handlers_disconnect_by_func(
      G_OBJECT(item->browser),
      G_CALLBACK(inf_gtk_browser_model_add_node_cb),
      model
    );

    g_signal_handlers_disconnect_by_func(
      G_OBJECT(item->browser),
      G_CALLBACK(inf_gtk_browser_model_remove_node_cb),
      model
    );

    g_object_unref(G_OBJECT(item->browser));
  }

  g_slice_free(InfGtkBrowserModelItem, item);
}

/*
 * GObject overrides
 */

static void
inf_gtk_browser_model_init(GTypeInstance* instance,
                           gpointer g_class)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;

  model = INF_GTK_BROWSER_MODEL(instance);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  priv->stamp = g_random_int();
  priv->discoveries = NULL;
  priv->first_item = NULL;
  priv->last_item = NULL;
}

static void
inf_gtk_browser_model_dispose(GObject* object)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;
  GSList* item;

  model = INF_GTK_BROWSER_MODEL(object);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  while(priv->first_item != NULL)
    inf_gtk_browser_model_remove_item(model, priv->first_item);
  g_assert(priv->last_item == NULL);

  for(item = priv->discoveries; item != NULL; item = g_slist_next(item))
  {
    g_signal_handlers_disconnect_by_func(
      G_OBJECT(item->data),
      G_CALLBACK(inf_gtk_browser_model_discovered_cb),
      model
    );

    g_signal_handlers_disconnect_by_func(
      G_OBJECT(item->data),
      G_CALLBACK(inf_gtk_browser_model_undiscovered_cb),
      model
    );

    g_object_unref(G_OBJECT(item->data));
  }

  g_slist_free(priv->discoveries);
  priv->discoveries = NULL;

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
inf_gtk_browser_model_set_property(GObject* object,
                                   guint prop_id,
                                   const GValue* value,
                                   GParamSpec* pspec)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;

  model = INF_GTK_BROWSER_MODEL(object);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  switch(prop_id)
  {
  case PROP_CONNECTION_MANAGER: 
    g_assert(priv->connection_manager == NULL); /* construct only */
    priv->connection_manager =
      INF_CONNECTION_MANAGER(g_value_dup_object(value));
  
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
inf_gtk_browser_model_get_property(GObject* object,
                                   guint prop_id,
                                   GValue* value,
                                   GParamSpec* pspec)
{
  InfGtkBrowserModel* model;
  InfGtkBrowserModelPrivate* priv;

  model = INF_GTK_BROWSER_MODEL(object);
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

  switch(prop_id)
  {
  case PROP_CONNECTION_MANAGER:
    g_value_set_object(value, G_OBJECT(priv->connection_manager));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/*
 * GtkTreeModel implementation
 */

static GtkTreeModelFlags
inf_gtk_browser_model_tree_model_get_flags(GtkTreeModel* model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
inf_gtk_browser_model_tree_model_get_n_columns(GtkTreeModel* model)
{
  return INF_GTK_BROWSER_MODEL_NUM_COLS;
}

static GType
inf_gtk_browser_model_tree_model_get_column_type(GtkTreeModel* model,
                                                 gint index)
{
  switch(index)
  {
  case INF_GTK_BROWSER_MODEL_COL_DISCOVERY_INFO:
    return G_TYPE_POINTER;
  case INF_GTK_BROWSER_MODEL_COL_BROWSER:
    return INFC_TYPE_BROWSER;
  case INF_GTK_BROWSER_MODEL_COL_STATUS:
    return INF_GTK_TYPE_BROWSER_MODEL_STATUS;
  case INF_GTK_BROWSER_MODEL_COL_ERROR:
    return G_TYPE_POINTER;
  case INF_GTK_BROWSER_MODEL_COL_NODE:
    return INFC_TYPE_BROWSER_ITER;
  default:
    g_assert_not_reached();
    return G_TYPE_INVALID;
  }
}

static gboolean
inf_gtk_browser_model_tree_model_get_iter(GtkTreeModel* model,
                                          GtkTreeIter* iter,
                                          GtkTreePath* path)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfcBrowserIter browser_iter;
  gint* indices;

  guint i;
  guint n;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  if(gtk_tree_path_get_depth(path) == 0) return FALSE;

  indices = gtk_tree_path_get_indices(path);
  n = indices[0];

  i = 0;
  for(item = priv->first_item; item != NULL && i < n; item = item->next)
    ++ i;

  if(i != n) return FALSE;
  g_assert(item != NULL);

  /* Depth 1 */
  if(gtk_tree_path_get_depth(path) == 1)
  {
    iter->stamp = priv->stamp;
    iter->user_data = item;
    iter->user_data2 = GUINT_TO_POINTER(0);
    iter->user_data3 = NULL;
    return TRUE;
  }

  if(item->browser != NULL) return FALSE;
  infc_browser_iter_get_root(item->browser, &browser_iter);
  
  for(n = 1; n < (guint)gtk_tree_path_get_depth(path); ++ n)
  {
    if(infc_browser_iter_get_explored(item->browser, &browser_iter) == FALSE)
      return FALSE;

    if(infc_browser_iter_get_child(item->browser, &browser_iter) == FALSE)
      return FALSE;

    for(i = 0; i < (guint)indices[n]; ++ i)
    {
      if(infc_browser_iter_get_next(item->browser, &browser_iter) == FALSE)
        return FALSE;
    }
  }

  iter->stamp = priv->stamp;
  iter->user_data = item;
  iter->user_data2 = GUINT_TO_POINTER(browser_iter.node_id);
  iter->user_data3 = browser_iter.node;
  return TRUE;
}

/* TODO: We can also use gtk_tree_path_prepend_index and do tail
 * recursion. We should find out which is faster. */
static void
inf_gtk_browser_model_tree_model_get_path_impl(InfGtkBrowserModel* model,
                                               InfGtkBrowserModelItem* item,
                                               InfcBrowserIter* iter,
                                               GtkTreePath* path)
{
  InfGtkBrowserModelPrivate* priv;
  InfcBrowserIter cur_iter;
  InfGtkBrowserModelItem* cur;
  gboolean result;
  guint n;
  
  cur_iter = *iter;
  if(infc_browser_iter_get_parent(item->browser, &cur_iter) == FALSE)
  {
    priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);

    /* We are on top level, but still need to find the item index */
    n = 0;
    for(cur = priv->first_item; cur != item; cur = cur->next)
      ++ n;

    gtk_tree_path_append_index(path, n);
  }
  else
  {
    inf_gtk_browser_model_tree_model_get_path_impl(
      model,
      item,
      &cur_iter,
      path
    );

    result = infc_browser_iter_get_child(item->browser, &cur_iter);
    g_assert(result == TRUE);

    while(cur_iter.node_id != iter->node_id)
    {
      result = infc_browser_iter_get_next(item->browser, &cur_iter);
      g_assert(result == TRUE);
      ++ n;
    }

    gtk_tree_path_append_index(path, n);
  }
}

static GtkTreePath*
inf_gtk_browser_model_tree_model_get_path(GtkTreeModel* model,
                                          GtkTreeIter* iter)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  GtkTreePath* path;
  InfcBrowserIter browser_iter;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(iter->stamp == priv->stamp);
  g_assert(iter->user_data != NULL);
  
  item = (InfGtkBrowserModelItem*)iter->user_data;

  path = gtk_tree_path_new();
  browser_iter.node_id = GPOINTER_TO_UINT(iter->user_data2);
  browser_iter.node = iter->user_data3;

  if(browser_iter.node == NULL)
    infc_browser_iter_get_root(item->browser, &browser_iter);

  inf_gtk_browser_model_tree_model_get_path_impl(
    INF_GTK_BROWSER_MODEL(model),
    item,
    &browser_iter,
    path
  );

  return path;
}

static void
inf_gtk_browser_model_tree_model_get_value(GtkTreeModel* model,
                                           GtkTreeIter* iter,
                                           gint column,
                                           GValue* value)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfcBrowserIter browser_iter;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(iter->stamp == priv->stamp);

  item = (InfGtkBrowserModelItem*)iter->user_data;
  browser_iter.node_id = GPOINTER_TO_UINT(iter->user_data2);
  browser_iter.node = iter->user_data3;
  
  switch(column)
  {
  case INF_GTK_BROWSER_MODEL_COL_DISCOVERY_INFO:
    g_value_init(value, G_TYPE_POINTER);
    g_value_set_pointer(value, item->info);
    break;
  case INF_GTK_BROWSER_MODEL_COL_BROWSER:
    g_value_init(value, INFC_TYPE_BROWSER);
    g_value_set_object(value, G_OBJECT(item->browser));
    break;
  case INF_GTK_BROWSER_MODEL_COL_STATUS:
    g_assert(browser_iter.node == NULL); /* only toplevel */
    g_value_init(value, INF_GTK_TYPE_BROWSER_MODEL_STATUS);
    g_value_set_enum(value, item->status);
    break;
  case INF_GTK_BROWSER_MODEL_COL_ERROR:
    g_assert(browser_iter.node == NULL); /* only toplevel */
    g_value_init(value, G_TYPE_POINTER);
    g_value_set_pointer(value, item->error);
    break;
  case INF_GTK_BROWSER_MODEL_COL_NODE:
    if(browser_iter.node == NULL)
    {
      /* get root node, if available */
      g_assert(item->browser != NULL);
      infc_browser_iter_get_root(item->browser, &browser_iter);
    }

    g_value_init(value, INFC_TYPE_BROWSER_ITER);
    g_value_set_boxed(value, &browser_iter);
    break;
  default:
    g_assert_not_reached();
    break;
  }
}

static gboolean
inf_gtk_browser_model_tree_model_iter_next(GtkTreeModel* model,
                                           GtkTreeIter* iter)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfcBrowserIter browser_iter;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(iter->stamp == priv->stamp);

  item = (InfGtkBrowserModelItem*)iter->user_data;
  browser_iter.node_id = GPOINTER_TO_UINT(iter->user_data2);
  browser_iter.node = iter->user_data3;

  if(browser_iter.node == NULL)
  {
    if(item->next == NULL)
      return FALSE;

    iter->user_data = item->next;
    return TRUE;
  }
  else
  {
    if(infc_browser_iter_get_next(item->browser, &browser_iter) == FALSE)
      return FALSE;

    iter->user_data2 = GUINT_TO_POINTER(browser_iter.node_id);
    iter->user_data3 = browser_iter.node;
    return TRUE;
  }
}

static gboolean
inf_gtk_browser_model_tree_model_iter_children(GtkTreeModel* model,
                                               GtkTreeIter* iter,
                                               GtkTreeIter* parent)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfcBrowserIter browser_iter;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(parent->stamp == priv->stamp);

  if(parent == NULL)
  {
    if(priv->first_item == NULL)
      return FALSE;

    iter->stamp = priv->stamp;
    iter->user_data = priv->first_item;
    iter->user_data2 = GUINT_TO_POINTER(0);
    iter->user_data3 = NULL;
    return TRUE;
  }
  else
  {
    item = (InfGtkBrowserModelItem*)parent->user_data;
    browser_iter.node_id = GPOINTER_TO_UINT(parent->user_data2);
    browser_iter.node = parent->user_data3;

    if(infc_browser_iter_get_explored(item->browser, &browser_iter) == FALSE)
      return FALSE;

    if(infc_browser_iter_get_child(item->browser, &browser_iter) == FALSE)
      return FALSE;

    iter->stamp = priv->stamp;
    iter->user_data = item;
    iter->user_data2 = GUINT_TO_POINTER(browser_iter.node_id);
    iter->user_data3 = browser_iter.node;
    return TRUE;
  }
}

static gboolean
inf_gtk_browser_model_tree_model_iter_has_child(GtkTreeModel* model,
                                                GtkTreeIter* iter)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfcBrowserIter browser_iter;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(iter->stamp == priv->stamp);

  item = (InfGtkBrowserModelItem*)iter->user_data;
  browser_iter.node_id = GPOINTER_TO_UINT(iter->user_data2);
  browser_iter.node = iter->user_data3;

  if(browser_iter.node == NULL)
    infc_browser_iter_get_root(item->browser, &browser_iter);

  if(infc_browser_iter_get_explored(item->browser, &browser_iter) == FALSE)
    return FALSE;

  return infc_browser_iter_get_child(item->browser, &browser_iter);
}

static gint
inf_gtk_browser_model_tree_model_iter_n_children(GtkTreeModel* model,
                                                 GtkTreeIter* iter)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfGtkBrowserModelItem* cur;
  InfcBrowserIter browser_iter;
  gboolean result;
  guint n;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(iter->stamp == priv->stamp);

  if(iter == NULL)
  {
    n = 0;
    for(cur = priv->first_item; cur != NULL; cur = cur->next)
      ++ n;

    return n;
  }
  else
  {
    item = (InfGtkBrowserModelItem*)iter->user_data;
    browser_iter.node_id = GPOINTER_TO_UINT(iter->user_data2);
    browser_iter.node = iter->user_data3;

    if(browser_iter.node == NULL)
      infc_browser_iter_get_root(item->browser, &browser_iter);

    if(infc_browser_iter_get_explored(item->browser, &browser_iter) == FALSE)
      return 0;

    n = 0;
    for(result = infc_browser_iter_get_child(item->browser, &browser_iter);
        result == TRUE;
        result = infc_browser_iter_get_next(item->browser, &browser_iter))
    {
      ++ n;
    }

    return n;
  }
}

static gboolean
inf_gtk_browser_model_tree_model_iter_nth_child(GtkTreeModel* model,
                                                GtkTreeIter* iter,
                                                GtkTreeIter* parent,
                                                gint n)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfGtkBrowserModelItem* cur;
  InfcBrowserIter browser_iter;
  guint i;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(parent->stamp == priv->stamp);

  if(parent == NULL)
  {
    cur = priv->first_item;
    if(cur == NULL) return FALSE;

    for(i = 0; i < (guint)n; ++ i)
    {
      cur = cur->next;
      if(cur == NULL) return FALSE;
    }

    iter->stamp = priv->stamp;
    iter->user_data = cur;
    iter->user_data2 = GUINT_TO_POINTER(0);
    iter->user_data3 = NULL;
    return TRUE;
  }
  else
  {
    item = (InfGtkBrowserModelItem*)parent->user_data;
    browser_iter.node_id = GPOINTER_TO_UINT(parent->user_data2);
    browser_iter.node = parent->user_data3;

    if(infc_browser_iter_get_explored(item->browser, &browser_iter) == FALSE)
      return FALSE;

    if(infc_browser_iter_get_child(item->browser, &browser_iter) == FALSE)
      return FALSE;

    for(i = 0; i < (guint)n; ++ i)
    {
      if(infc_browser_iter_get_next(item->browser, &browser_iter) == FALSE)
        return FALSE;
    }

    iter->stamp = priv->stamp;
    iter->user_data = item;
    iter->user_data2 = GUINT_TO_POINTER(browser_iter.node_id);
    iter->user_data3 = browser_iter.node;
    return TRUE;
  }
}

static gboolean
inf_gtk_browser_model_tree_model_iter_parent(GtkTreeModel* model,
                                             GtkTreeIter* iter,
                                             GtkTreeIter* child)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  InfcBrowserIter browser_iter;
  gboolean result;

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_assert(child->stamp == priv->stamp);

  item = (InfGtkBrowserModelItem*)child->user_data;
  browser_iter.node_id = GPOINTER_TO_UINT(child->user_data2);
  browser_iter.node = child->user_data3;

  if(browser_iter.node == NULL)
    return FALSE;

  result = infc_browser_iter_get_parent(item->browser, &browser_iter);
  g_assert(result == TRUE);

  iter->stamp = priv->stamp;
  iter->user_data = item;
  iter->user_data2 = GUINT_TO_POINTER(browser_iter.node_id);
  iter->user_data3 = browser_iter.node;

  /* Root node */
  if(browser_iter.node_id == 0)
    iter->user_data3 = NULL;

  return TRUE;
}

/*
 * GType registration
 */

static void
inf_gtk_browser_model_class_init(gpointer g_class,
                                 gpointer class_data)
{
  GObjectClass* object_class;
  object_class = G_OBJECT_CLASS(g_class);

  parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(g_class));
  g_type_class_add_private(g_class, sizeof(InfGtkBrowserModelPrivate));

  object_class->dispose = inf_gtk_browser_model_dispose;
  object_class->set_property = inf_gtk_browser_model_set_property;
  object_class->get_property = inf_gtk_browser_model_get_property;

  g_object_class_install_property(
    object_class,
    PROP_CONNECTION_MANAGER,
    g_param_spec_object(
      "connection-manager",
      "Connection manager", 
      "The connection manager used for browsing remote directories",
      INF_TYPE_CONNECTION_MANAGER,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    )
  );
}

static void
inf_gtk_browser_model_tree_model_init(gpointer g_iface,
                                      gpointer iface_data)
{
  GtkTreeModelIface* iface;
  iface = (GtkTreeModelIface*)g_iface;

  iface->get_flags = inf_gtk_browser_model_tree_model_get_flags;
  iface->get_n_columns = inf_gtk_browser_model_tree_model_get_n_columns;
  iface->get_column_type = inf_gtk_browser_model_tree_model_get_column_type;
  iface->get_iter = inf_gtk_browser_model_tree_model_get_iter;
  iface->get_path = inf_gtk_browser_model_tree_model_get_path;
  iface->get_value = inf_gtk_browser_model_tree_model_get_value;
  iface->iter_next = inf_gtk_browser_model_tree_model_iter_next;
  iface->iter_children = inf_gtk_browser_model_tree_model_iter_children;
  iface->iter_has_child = inf_gtk_browser_model_tree_model_iter_has_child;
  iface->iter_n_children = inf_gtk_browser_model_tree_model_iter_n_children;
  iface->iter_nth_child = inf_gtk_browser_model_tree_model_iter_nth_child;
  iface->iter_parent = inf_gtk_browser_model_tree_model_iter_parent;
}

GType
inf_gtk_browser_model_status_get_type(void)
{
  static GType browser_model_status_type = 0;

  if(!browser_model_status_type)
  {
    static const GEnumValue browser_model_status_values[] = {
      {
        INF_GTK_BROWSER_MODEL_DISCOVERED,
        "INF_GTK_BROWSER_MODEL_DISCOVERED",
        "discovered"
      }, {
        INF_GTK_BROWSER_MODEL_RESOLVING,
        "INF_GTK_BROWSER_MODEL_RESOLVING",
        "resolving"
      }, {
        INF_GTK_BROWSER_MODEL_CONNECTING,
        "INF_GTK_BROWSER_MODEL_CONNECTING",
        "connecting"
      }, {
        INF_GTK_BROWSER_MODEL_CONNECTED,
        "INF_GTK_BROWSER_MODEL_CONNECTED",
        "connected"
      }, {
        INF_GTK_BROWSER_MODEL_ERROR,
        "INF_GTK_BROWSER_MODEL_ERROR",
        "error"
      }
    };

    browser_model_status_type = g_enum_register_static(
      "InfGtkBrowserModelStatus",
      browser_model_status_values
    );
  }

  return browser_model_status_type;
}

GType
inf_gtk_browser_model_get_type(void)
{
  static GType browser_model_type = 0;

  if(!browser_model_type)
  {
    static const GTypeInfo browser_model_type_info = {
      sizeof(InfGtkBrowserModelClass),    /* class_size */
      NULL,                               /* base_init */
      NULL,                               /* base_finalize */
      inf_gtk_browser_model_class_init,   /* class_init */
      NULL,                               /* class_finalize */
      NULL,                               /* class_data */
      sizeof(InfGtkBrowserModel),         /* instance_size */
      0,                                  /* n_preallocs */
      inf_gtk_browser_model_init,         /* instance_init */
      NULL                                /* value_table */
    };

    static const GInterfaceInfo tree_model_info = {
      inf_gtk_browser_model_tree_model_init,
      NULL,
      NULL
    };

    browser_model_type = g_type_register_static(
      G_TYPE_OBJECT,
      "InfGtkBrowserModel",
      &browser_model_type_info,
      0
    );

    g_type_add_interface_static(
      browser_model_type,
      GTK_TYPE_TREE_MODEL,
      &tree_model_info
    );
  }

  return browser_model_type;
}

/*
 * Public API.
 */

/** inf_gtk_browser_model_new:
 *
 * Creates a new #InfGtkBrowserModel.
 *
 * Return Value: A new #InfGtkBrowserModel.
 **/
InfGtkBrowserModel*
inf_gtk_browser_model_new(void)
{
  GObject* object;
  object = g_object_new(INF_GTK_TYPE_BROWSER_MODEL, NULL);
  return INF_GTK_BROWSER_MODEL(object);
}

/** inf_gtk_browser_model_add_discovery:
 *
 * @model: A #InfGtkBrowserModel.
 * @discovery: A #InfDiscovery not yet added to @model.
 *
 * Adds @discovery to @model. The model will then show up discovered
 * servers.
 **/
void
inf_gtk_browser_model_add_discovery(InfGtkBrowserModel* model,
                                    InfDiscovery* discovery)
{
  InfGtkBrowserModelPrivate* priv;
  GSList* discovered;
  GSList* item;

  g_return_if_fail(INF_GTK_IS_BROWSER_MODEL(model));
  g_return_if_fail(INF_IS_DISCOVERY(discovery));

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  g_return_if_fail(g_slist_find(priv->discoveries, discovery) == NULL);

  g_object_ref(G_OBJECT(discovery));
  priv->discoveries = g_slist_prepend(priv->discoveries, discovery);

  g_signal_connect(
    G_OBJECT(discovery),
    "discovered",
    G_CALLBACK(inf_gtk_browser_model_discovered_cb),
    model
  );

  g_signal_connect(
    G_OBJECT(discovery),
    "undiscovered",
    G_CALLBACK(inf_gtk_browser_model_undiscovered_cb),
    model
  );

  discovered = inf_discovery_get_discovered(discovery, "_infinote._tcp");
  for(item = discovered; item != NULL; item = g_slist_next(item))
  {
    inf_gtk_browser_model_add_item(
      model,
      discovery,
      (InfDiscoveryInfo*)item->data,
      NULL
    );
  }

  g_slist_free(discovered);
}

/** inf_gtk_browser_model_add_connection:
 *
 * @model: A #InfGtkBrowserModel.
 * @connection: A #InfXmlConnection.
 *
 * This function adds a connection to the @model. @model will show up
 * an item for the connection if there is not already one. This allows to
 * browse the explored parts of the directory of the remote site.
 *
 * @connection must be in %INF_XML_CONNECTION_OPEN or
 * %INF_XML_CONNECTION_OPENING status.
 **/
void
inf_gtk_browser_model_add_connection(InfGtkBrowserModel* model,
                                     InfXmlConnection* connection)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;

  g_return_if_fail(INF_GTK_IS_BROWSER_MODEL(model));
  g_return_if_fail(INF_IS_XML_CONNECTION(connection));

  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  item = inf_gtk_browser_model_find_item_by_connection(model, connection);

  if(item == NULL)
    inf_gtk_browser_model_add_item(model, NULL, NULL, connection);
}

/** inf_gtk_browser_model_resolve:
 *
 * @model: A #InfGtkBrowserModel.
 * @discovery: A #InfDiscovery added to @model.
 * @info: A #InfDiscoveryInfo discovered by @discovery.
 *
 * Resolves @info and adds the resulting connection to the model. If that
 * connection is already contained, the original (newly resolved) entry
 * is removed in favor of the existing entry whose browser might already
 * have explored (parts of) the server's directory. */
void
inf_gtk_browser_model_resolve(InfGtkBrowserModel* model,
                              InfDiscovery* discovery,
                              InfDiscoveryInfo* info)
{
  InfGtkBrowserModelPrivate* priv;
  InfGtkBrowserModelItem* item;
  GtkTreeIter tree_iter;
  GtkTreePath* path;

  g_return_if_fail(INF_GTK_IS_BROWSER_MODEL(model));
  g_return_if_fail(INF_IS_DISCOVERY(discovery));
  g_return_if_fail(info != NULL);
  
  priv = INF_GTK_BROWSER_MODEL_PRIVATE(model);
  item = inf_gtk_browser_model_find_item_by_discovery_info(model, info);

  g_return_if_fail(item != NULL);

  /* TODO: Also allow resolve in error status to retry. Clear error before
   * retry though. */
  g_return_if_fail(item->status == INF_GTK_BROWSER_MODEL_DISCOVERED);

  item->status = INF_GTK_BROWSER_MODEL_RESOLVING;
  tree_iter.stamp = priv->stamp;
  tree_iter.user_data = item;
  tree_iter.user_data2 = GUINT_TO_POINTER(0);
  tree_iter.user_data3 = NULL;

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &tree_iter);
  gtk_tree_model_row_changed(GTK_TREE_MODEL(model), path, &tree_iter);
  gtk_tree_path_free(path);

  inf_discovery_resolve(
    discovery,
    info,
    inf_gtk_browser_model_resolv_complete_func,
    inf_gtk_browser_model_resolv_error_func,
    model
  );
}

/* vim:set et sw=2 ts=2: */

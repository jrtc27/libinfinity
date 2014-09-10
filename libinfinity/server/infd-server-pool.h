/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007-2014 Armin Burgmeier <armin@arbur.net>
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
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef __INFD_SERVER_POOL_H__
#define __INFD_SERVER_POOL_H__

#include <libinfinity/server/infd-directory.h>
#include <libinfinity/server/infd-xmpp-server.h>
#include <libinfinity/server/infd-xml-server.h>
#include <libinfinity/common/inf-local-publisher.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define INFD_TYPE_SERVER_POOL                 (infd_server_pool_get_type())
#define INFD_SERVER_POOL(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), INFD_TYPE_SERVER_POOL, InfdServerPool))
#define INFD_SERVER_POOL_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), INFD_TYPE_SERVER_POOL, InfdServerPoolClass))
#define INFD_IS_SERVER_POOL(obj)              (G_TYPE_CHECK_INSTANCE_TYPE((obj), INFD_TYPE_SERVER_POOL))
#define INFD_IS_SERVER_POOL_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), INFD_TYPE_SERVER_POOL))
#define INFD_SERVER_POOL_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), INFD_TYPE_SERVER_POOL, InfdServerPoolClass))

typedef struct _InfdServerPool InfdServerPool;
typedef struct _InfdServerPoolClass InfdServerPoolClass;

/**
 * InfdServerPoolClass:
 *
 * This structure does not contain any public fields.
 */
struct _InfdServerPoolClass {
  /*< private >*/
  GObjectClass parent_class;
};

/**
 * InfdServerPool:
 *
 * #InfdServerPool is an opaque data type. You should only access it via the
 * public API functions.
 */
struct _InfdServerPool {
  /*< private >*/
  GObject parent;
};

/**
 * InfdServerPoolForeachServerFunc:
 * @server: The currently iterated server.
 * @user_data: Additional data passed to infd_server_pool_foreach_server().
 *
 * This is the callback signature of the callback passed to
 * infd_server_pool_foreach_server().
 */
typedef void(*InfdServerPoolForeachServerFunc)(InfdXmlServer* server,
                                               gpointer user_data);

GType
infd_server_pool_get_type(void) G_GNUC_CONST;

InfdServerPool*
infd_server_pool_new(InfdDirectory* directory);

void
infd_server_pool_add_server(InfdServerPool* server_pool,
                            InfdXmlServer* server);

void
infd_server_pool_add_local_publisher(InfdServerPool* server_pool,
                                     InfdXmppServer* server,
                                     InfLocalPublisher* publisher);

void
infd_server_pool_remove_server(InfdServerPool* server_pool,
                               InfdXmlServer* server);

void
infd_server_pool_foreach_server(InfdServerPool* server_pool,
                                InfdServerPoolForeachServerFunc func,
                                gpointer user_data);

G_END_DECLS

#endif /* __INFD_SERVER_POOL_H__ */

/* vim:set et sw=2 ts=2: */

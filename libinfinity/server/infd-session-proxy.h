/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007-2015 Armin Burgmeier <armin@arbur.net>
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

#ifndef __INFD_SESSION_PROXY_H__
#define __INFD_SESSION_PROXY_H__

#include <libinfinity/common/inf-session.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define INFD_TYPE_SESSION_PROXY                 (infd_session_proxy_get_type())
#define INFD_SESSION_PROXY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), INFD_TYPE_SESSION_PROXY, InfdSessionProxy))
#define INFD_SESSION_PROXY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), INFD_TYPE_SESSION_PROXY, InfdSessionProxyClass))
#define INFD_IS_SESSION_PROXY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE((obj), INFD_TYPE_SESSION_PROXY))
#define INFD_IS_SESSION_PROXY_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), INFD_TYPE_SESSION_PROXY))
#define INFD_SESSION_PROXY_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), INFD_TYPE_SESSION_PROXY, InfdSessionProxyClass))

typedef struct _InfdSessionProxy InfdSessionProxy;
typedef struct _InfdSessionProxyClass InfdSessionProxyClass;

/**
 * InfdSessionProxyClass:
 * @add_subscription: Default signal handler for the
 * #InfdSessionProxy::add-subscription signal.
 * @remove_subscription: Default signal handler for the
 * #InfdSessionProxy::remove-subscription signal.
 * @reject_user_join: Default signal handler for the
 * #InfdSessionProxy::reject-user-join signal.
 *
 * This structure contains the default signal handlers of the
 * #InfdSessionProxy class.
 */
struct _InfdSessionProxyClass {
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/

  /* Signals */
  void (*add_subscription)(InfdSessionProxy* proxy,
                           InfXmlConnection* connection,
                           guint seq_id);

  void (*remove_subscription)(InfdSessionProxy* proxy,
                              InfXmlConnection* connection);

  gboolean (*reject_user_join)(InfdSessionProxy* proxy,
                               InfXmlConnection* connection,
                               const GArray* user_properties,
                               InfUser* rejoin_user);
};

/**
 * InfdSessionProxy:
 *
 * #InfdSessionProxy is an opaque data type. You should only access it via the
 * public API functions.
 */
struct _InfdSessionProxy {
  /*< private >*/
  GObject parent;
};

GType
infd_session_proxy_get_type(void) G_GNUC_CONST;

void
infd_session_proxy_subscribe_to(InfdSessionProxy* proxy,
                                InfXmlConnection* connection,
                                guint seq_id,
                                gboolean synchronize);

void
infd_session_proxy_unsubscribe(InfdSessionProxy* proxy,
                               InfXmlConnection* connection);

gboolean
infd_session_proxy_has_subscriptions(InfdSessionProxy* proxy);

gboolean
infd_session_proxy_is_subscribed(InfdSessionProxy* proxy,
                                 InfXmlConnection* connection);

gboolean
infd_session_proxy_is_idle(InfdSessionProxy* proxy);

G_END_DECLS

#endif /* __INFD_SESSION_PROXY_H__ */

/* vim:set et sw=2 ts=2: */

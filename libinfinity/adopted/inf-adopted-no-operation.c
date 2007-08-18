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

#include <libinfinity/adopted/inf-adopted-no-operation.h>
#include <libinfinity/adopted/inf-adopted-operation.h>

static GObjectClass* parent_class;

static void
inf_adopted_no_operation_class_init(gpointer g_class,
                                    gpointer class_data)
{
  parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(g_class));
}

static InfAdoptedOperation*
inf_adopted_no_operation_transform(InfAdoptedOperation* operation,
                                   InfAdoptedOperation* against)
{
  return INF_ADOPTED_OPERATION(inf_adopted_no_operation_new());
}

#if 0
static InfAdoptedOperation*
inf_adopted_no_operation_copy(InfAdoptedOperation* operation)
{
  return INF_ADOPTED_OPERATION(inf_adopted_no_operation_new());
}
#endif

static InfAdoptedOperationFlags
inf_adopted_no_operation_get_flags(InfAdoptedOperation* operation)
{
  return 0;
}

static void
inf_adopted_no_operation_apply(InfAdoptedOperation* operation,
                               InfAdoptedUser* by,
                               InfBuffer* buffer)
{
  /* Does nothing */
}

static gboolean
inf_adopted_no_operation_is_reversible(InfAdoptedOperation* operation)
{
  return TRUE;
}

static InfAdoptedOperation*
inf_adopted_no_operation_revert(InfAdoptedOperation* operation)
{
  return INF_ADOPTED_OPERATION(inf_adopted_no_operation_new());
}

static void
inf_adopted_no_operation_operation_init(gpointer g_iface,
                                           gpointer iface_data)
{
  InfAdoptedOperationIface* iface;
  iface = (InfAdoptedOperationIface*)g_iface;

  iface->transform = inf_adopted_no_operation_transform;
  /*iface->copy = inf_adopted_no_operation_copy;*/
  iface->get_flags = inf_adopted_no_operation_get_flags;
  iface->apply = inf_adopted_no_operation_apply;
  iface->is_reversible = inf_adopted_no_operation_is_reversible;
  iface->revert = inf_adopted_no_operation_revert;

  /* should never be called because no_operation is always reversible */
  iface->make_reversible = NULL;
}

GType
inf_adopted_no_operation_get_type(void)
{
  static GType no_operation_type = 0;

  if(!no_operation_type)
  {
    static const GTypeInfo no_operation_type_info = {
      sizeof(InfAdoptedNoOperationClass),    /* class_size */
      NULL,                                  /* base_init */
      NULL,                                  /* base_finalize */
      inf_adopted_no_operation_class_init,   /* class_init */
      NULL,                                  /* class_finalize */
      NULL,                                  /* class_data */
      sizeof(InfAdoptedNoOperation),         /* instance_size */
      0,                                     /* n_preallocs */
      NULL,                                  /* instance_init */
      NULL                                   /* value_table */
    };

    static const GInterfaceInfo operation_info = {
      inf_adopted_no_operation_operation_init,
      NULL,
      NULL
    };

    no_operation_type = g_type_register_static(
      G_TYPE_OBJECT,
      "InfAdoptedNoOperation",
      &no_operation_type_info,
      0
    );

    g_type_add_interface_static(
      no_operation_type,
      INF_ADOPTED_TYPE_OPERATION,
      &operation_info
    );
  }

  return no_operation_type;
}

/** inf_adopted_no_operation_new:
 *
 * Creates a new #InfAdoptedNoOperation. A no operation is an operation
 * that does nothing, but might be the result of a transformation.
 *
 * Return Value: A new #InfAdoptedNoOperation.
 **/
InfAdoptedNoOperation*
inf_adopted_no_operation_new(void)
{
  GObject* object;
  object = g_object_new(INF_ADOPTED_TYPE_NO_OPERATION, NULL);
  return INF_ADOPTED_NO_OPERATION(object);
}

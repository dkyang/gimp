/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationequalize.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "core/gimphistogram.h"

#include "gimpoperationequalize.h"


enum
{
  PROP_0,
  PROP_HISTOGRAM
};


static void   gimp_operation_equalize_finalize     (GObject    *object);
static void   gimp_operation_equalize_get_property (GObject    *object,
                                                    guint       property_id,
                                                    GValue     *value,
                                                    GParamSpec *pspec);
static void   gimp_operation_equalize_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);

static gboolean gimp_operation_equalize_process (GeglOperation       *operation,
                                                 void                *in_buf,
                                                 void                *out_buf,
                                                 glong                samples,
                                                 const GeglRectangle *roi,
                                                 gint                 level);


G_DEFINE_TYPE (GimpOperationEqualize, gimp_operation_equalize,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_equalize_parent_class


static void
gimp_operation_equalize_class_init (GimpOperationEqualizeClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize       = gimp_operation_equalize_finalize;
  object_class->set_property   = gimp_operation_equalize_set_property;
  object_class->get_property   = gimp_operation_equalize_get_property;

  operation_class->name        = "gimp:equalize";
  operation_class->categories  = "color";
  operation_class->description = "GIMP Equalize operation";

  point_class->process         = gimp_operation_equalize_process;

  g_object_class_install_property (object_class, PROP_HISTOGRAM,
                                   g_param_spec_pointer ("histogram",
                                                         "Histogram",
                                                         "The histogram",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_operation_equalize_init (GimpOperationEqualize *self)
{
}

static void
gimp_operation_equalize_finalize (GObject *object)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  if (self->histogram)
    {
      gimp_histogram_unref (self->histogram);
      self->histogram = NULL;
    }
}

static void
gimp_operation_equalize_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  switch (property_id)
    {
    case PROP_HISTOGRAM:
      g_value_set_pointer (value, self->histogram);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_equalize_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (object);

  switch (property_id)
    {
    case PROP_HISTOGRAM:
      if (self->histogram)
        gimp_histogram_unref (self->histogram);
      self->histogram = g_value_get_pointer (value);
      if (self->histogram)
        {
          gdouble pixels;
          gint    max;
          gint    k;

          gimp_histogram_ref (self->histogram);

          pixels = gimp_histogram_get_count (self->histogram,
                                             GIMP_HISTOGRAM_VALUE, 0, 255);

          if (gimp_histogram_n_channels (self->histogram) == 1 ||
              gimp_histogram_n_channels (self->histogram) == 2)
            max = 1;
          else
            max = 3;

          for (k = 0; k < 3; k++)
            {
              gdouble sum = 0;
              gint    i;

             for (i = 0; i < 256; i++)
                {
                  gdouble histi;

                  histi = gimp_histogram_get_channel (self->histogram, k, i);

                  sum += histi;

                  self->part[k][i] = sum / pixels;

                  if (max == 1)
                    {
                      self->part[1][i] = self->part[0][i];
                      self->part[2][i] = self->part[0][i];
                    }
                }
           }
        }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static inline float
gimp_operation_equalize_map (GimpOperationEqualize *self,
                             gint                   component,
                             gfloat                 value)
{
  gint index = (gint) CLAMP (value * 255.0, 0, 255);

  return self->part[component][index];
}

static gboolean
gimp_operation_equalize_process (GeglOperation       *operation,
                                 void                *in_buf,
                                 void                *out_buf,
                                 glong                samples,
                                 const GeglRectangle *roi,
                                 gint                 level)
{
  GimpOperationEqualize *self = GIMP_OPERATION_EQUALIZE (operation);
  gfloat                *src  = in_buf;
  gfloat                *dest = out_buf;

  while (samples--)
    {
      dest[RED]   = gimp_operation_equalize_map (self, RED,   src[RED]);
      dest[GREEN] = gimp_operation_equalize_map (self, GREEN, src[GREEN]);
      dest[BLUE]  = gimp_operation_equalize_map (self, BLUE,  src[BLUE]);
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
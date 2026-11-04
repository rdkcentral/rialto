// Compile-time subset of GLib's main loop API. Types and signatures match GLib 2.x.
// Used only to compile and link the JS binding on a machine without GLib.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int gboolean;
typedef unsigned int guint;
typedef int gint;
typedef void *gpointer;
typedef void (*GDestroyNotify)(gpointer data);
typedef gboolean (*GSourceFunc)(gpointer user_data);

#define TRUE 1
#define FALSE 0

#define G_PRIORITY_DEFAULT 0
#define G_SOURCE_REMOVE FALSE
#define G_SOURCE_CONTINUE TRUE

guint g_idle_add_full(gint priority, GSourceFunc function, gpointer data, GDestroyNotify notify);
gboolean g_source_remove(guint tag);

#ifdef __cplusplus
}
#endif

#ifndef WIDGET_REGISTRY_H__
#define WIDGET_REGISTRY_H__

#include <iostream>
#include <gtk/gtk.h>
#include <list>
#include <map>
#include <boost/foreach.hpp>

template <class C> class WidgetRegistry {
  public:
    static typename std::map<GtkWidget*, C*> widget_map;

    static C* get_object(GtkWidget *widget) {
      GtkWidget* top_level_widget = get_toplevel_widget(widget);
      if (NULL != top_level_widget) {
        return widget_map[top_level_widget];
      } else {
        return NULL;
      }
    };

    static void set_widget(GtkWidget *widget, C *object) {
      widget_map[widget] = object;
    };

    static void forget_widget(GtkWidget *widget) {
      C *x;
      widget_map.erase(widget);
    };

    static std::list<C * > get_objects() {
      typedef std::pair<GtkWidget*, C*> entry_t;
      std::list<C * > result;
      BOOST_FOREACH(entry_t map_entry, widget_map) {
        result.push_back(map_entry.second);
      }
      return result;
    }

    static long count() {
      return widget_map.size();
    }
    
    // Other registry methods

   static GtkWidget *get_toplevel_widget (GtkWidget *widget) {
      GtkWidget *parent;

      const int maxLevels = 10000; // Don't try forever
      int i;
      GtkWidget *candidate_widget = widget;
      for (i=0; i<maxLevels; i++) {
        if (widget_map.count(candidate_widget)) {
	  return candidate_widget;
        }
        if (GTK_IS_MENU (candidate_widget)) {
          candidate_widget = gtk_menu_get_attach_widget (GTK_MENU (candidate_widget));
        } else {
          candidate_widget = gtk_widget_get_parent(candidate_widget);
        }
        if (candidate_widget == NULL) {
          break;
        }
      }
      return (NULL);
    }

   static std::string get_class(GtkWidget *widget) {
     GType Type = G_TYPE_FROM_INSTANCE(widget);
     const gchar *Name = g_type_name(Type);
     return std::string(Name);
   };
};
#endif // WIDGET_REGISTRY_H__

#ifndef WIDGET_REGISTRY_H__
#define WIDGET_REGISTRY_H__

#include <iostream>
#include <gtk/gtk.h>
#include <map>

template <class C> class WidgetRegistry {
  public:
  static typename std::map<GtkWidget*, C*> widget_map;

  static C* get_object(GtkWidget *widget) {
    GtkWidget* top_level_widget = get_toplevel_widget(widget);
    std::cout << "top_level_widget is " << (long) top_level_widget << std::endl;
    if (NULL != top_level_widget) {
      return widget_map[top_level_widget];
    } else {
      return NULL;
    }
  };

  static void set_widget(GtkWidget *widget, C *object) {
    std::cout << "set_widget " << (long) widget << std::endl;
    widget_map[widget] = object;
  };

  static void forget_widget(GtkWidget *widget) {
    C *x;
    widget_map.erase(widget);
  };

  static long count() {
    return widget_map.size();
  }
    
  // Other registry methods

 static GtkWidget *get_toplevel_widget (GtkWidget *widget) {
    GtkWidget *parent;

    const int maxLevels = 10000; // Don't try forever
    int i;
    GtkWidget *candidate_widget = widget;
    std::cout << "get_toplevel_widget " << (long) widget << std::endl;
    std::cout << "class is " << get_class(widget) << std::endl;
    for (i=0; i<maxLevels; i++) {
      std::cout << "checking candidate_widget " << (long) candidate_widget << std::endl;
      std::cout << "class is " << get_class(candidate_widget) << std::endl;
      if (widget_map.count(candidate_widget)) {
      std::cout << "candidate_widget is the one" << std::endl;
	return candidate_widget;
      }
      std::cout << "candidate_widget is not the one" << std::endl;
      if (GTK_IS_MENU (candidate_widget)) {
        std::cout << "candidate_widget is menu" << std::endl;
        candidate_widget = gtk_menu_get_attach_widget (GTK_MENU (candidate_widget));
      } else {
        std::cout << "candidate_widget is not menu" << std::endl;
        candidate_widget = gtk_widget_get_parent(candidate_widget);
      }
      if (candidate_widget == NULL) {
        std::cout << "**************candidate_widget is NULL" << std::endl;
        break;
      }
    }
    std::cout << "************** returning NULL" << std::endl;
    return (NULL);
  }

 static std::string get_class(GtkWidget *widget) {
   GType Type = G_TYPE_FROM_INSTANCE(widget);
   const gchar *Name = g_type_name(Type);
   return std::string(Name);
 };
};

#endif // WIDGET_REGISTRY_H__

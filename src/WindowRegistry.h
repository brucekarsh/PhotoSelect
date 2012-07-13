#ifndef WINDOW_REGISTRY_H__
#define WINDOW_REGISTRY_H__

#include <iostream>
#include <gtk/gtk.h>
#include <map>

template <class C> class WindowRegistry {
  public:
  static typename std::map<GtkWidget*, C*> windowMap;

  static C* getWindow(GtkWidget *widget) {
    C *window = NULL;
    GtkWidget* top_level_window = get_toplevel_widget(widget);
    typename std::map<GtkWidget*, C *>::iterator it =
      windowMap.find(GTK_WIDGET(top_level_window));
    if (windowMap.end() == it) {
      std::cout << "Cannot find window in the map" << std::endl;
    } else {
      window = it -> second;
    }
    return window;
  };

  static void setWindow(GtkWidget *widget, C *window) {
    windowMap[GTK_WIDGET(widget)] = window;
  };

  static void forgetWindow(GtkWidget *widget) {
    C *x;
    windowMap.erase(widget);
  };
    
  // Other registry methods

 static GtkWidget *get_toplevel_widget (GtkWidget *widget) {
    GtkWidget *parent;

    const int maxLevels = 10000; // Don't try forever
    int i;
    for (i=0; i<maxLevels; i++) {
      if (GTK_IS_MENU (widget)) {
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      } else {
        parent = gtk_widget_get_parent(widget);
      }
      if (parent == NULL) {
        break;
      }
      widget = parent;
    }
    if (i == maxLevels) {
      return 0;
    }
    return (widget);
  }

 static std::string get_class(GtkWidget *widget) {
   GType Type = G_TYPE_FROM_INSTANCE(widget);
   const gchar *Name = g_type_name(Type);
   return std::string(Name);
 };
};
#endif // WINDOW_REGISTRY_H__

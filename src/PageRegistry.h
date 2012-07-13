#ifndef PAGE_REGISTRY_H__
#define PAGE_REGISTRY_H__

#include <iostream>
#include <gtk/gtk.h>
#include <map>

template <class C> class PageRegistry {
  static std::map<GtkWidget*, C*> pageMap;

  public:
  static C *getPage(GtkWidget *widget) {
    C *page = NULL;
    GtkWidget* top_level_page = get_toplevel_page(widget);
    typename std::map<GtkWidget*, C *>::iterator it =
      pageMap.find(GTK_WIDGET(top_level_page));
    if (pageMap.end() == it) {
      std::cout << "Cannot find page in the map" << std::endl;
    } else {
      page = it -> second;
    }
    return page;
  }

  static void setPage(GtkWidget *widget, C *page) {
    pageMap[GTK_WIDGET(widget)] = page;
  }

  static void forgetPage(GtkWidget *widget) {
    pageMap.erase(widget);
  }
    
  // Other registry methods

   static GtkWidget *get_toplevel_page(GtkWidget *widget) {
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
       if (GTK_IS_NOTEBOOK(parent)) {
         return widget;
       }
       widget = parent;
     }
     return (NULL);
   }

 static std::string get_class(GtkWidget *widget) {
   GType Type = G_TYPE_FROM_INSTANCE(widget);
   const gchar *Name = g_type_name(Type);
   return std::string(Name);
 }
};
#endif // PAGE_REGISTRY_H__


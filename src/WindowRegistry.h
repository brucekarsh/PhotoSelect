#ifndef WINDOW_REGISTRY_H__
#define WINDOW_REGISTRY_H__

#include <iostream>
#include <gtk/gtk.h>
#include <map>
class PhotoSelectPage;
class BaseWindow;
class PreferencesWindow;
class ImportWindow;
class QueryWindow;
class OpenProjectWindow;
class NewProjectWindow;

/** Remembers a mapping from top-level windows to our objects so we can get find
**  our objects given a top-level window.
*/

class WindowRegistry {
    static std::map<GtkWidget*, PhotoSelectPage*> photoSelectPageMap;
    static std::map<GtkWindow*, BaseWindow*> baseWindowMap;
    static std::map<GtkWindow*, PreferencesWindow*> preferencesWindowMap;
    static std::map<GtkWindow*, ImportWindow*> importWindowMap;
    static std::map<GtkWindow*, QueryWindow*> queryWindowMap;
    static std::map<GtkWindow*, OpenProjectWindow*> openProjectWindowMap;
    static std::map<GtkWindow*, NewProjectWindow*> newProjectWindowMap;
  public:

    // PhotoSelectPage registry methods

    static PhotoSelectPage* getPhotoSelectPage(GtkWidget *widget) {
      PhotoSelectPage *photoSelectPage = 0;
      GtkWidget* top_level_page = get_toplevel_page(widget);
      std::map<GtkWidget*, PhotoSelectPage*>::iterator it =
        photoSelectPageMap.find(GTK_WIDGET(top_level_page));
      if (photoSelectPageMap.end() == it) {
        std::cout << "Cannot find PhotoSelect page in the photoSelectPageMap" << std::endl;
      } else {
        photoSelectPage = it -> second;
      }
      return photoSelectPage;
    }

    static void setPhotoSelectPage(GtkWidget *widget, PhotoSelectPage *photoSelectPage) {
      photoSelectPageMap[GTK_WIDGET(widget)] = photoSelectPage;
    }

    static void forgetPhotoSelectPage(GtkWidget *widget) {
      std::cout << "TODO forgetPhotoSelectPage WRITEME" << std::endl;
    };

    // BaseWindow registry methods

    static BaseWindow* getBaseWindow(GtkWidget *widget) {
      BaseWindow *baseWindow = 0;
      std::map<GtkWindow*, BaseWindow*>::iterator it =
        baseWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (baseWindowMap.end() == it) {
        std::cout << "Cannot find BaseWindow window in the baseWindowMap" << std::endl;
      } else {
        baseWindow = it -> second;
      }
      return baseWindow;
    }

    static void setBaseWindow(GtkWidget *widget, BaseWindow *baseWindow) {
      baseWindowMap[GTK_WINDOW(gtk_widget_get_toplevel(widget))] = baseWindow;
    }

    static void forgetBaseWindow(GtkWidget *widget) {
      std::cout << "TODO setBaseWindow WRITEME" << std::endl;
    }

    // PreferencesWindow registry methods

    static PreferencesWindow* getPreferencesWindow(GtkWidget *widget) {
      PreferencesWindow *preferencesWindow = 0;
      std::map<GtkWindow*, PreferencesWindow*>::iterator it =
        preferencesWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (preferencesWindowMap.end() == it) {
        std::cout << "Cannot find PreferencesWindow window in the preferencesWindowMap" << std::endl;
      } else {
        preferencesWindow = it -> second;
      }
      return preferencesWindow;
    }

    static void setPreferencesWindow(GtkWidget *widget, PreferencesWindow *preferencesWindow) {
      preferencesWindowMap[GTK_WINDOW(gtk_widget_get_toplevel(widget))] = preferencesWindow;
    }

    static void forgetPreferencesWindow(GtkWidget *widget) {
      std::cout << "TODO setPreferencesWindow WRITEME" << std::endl;
    }

    // ImportWindow registry methods

    static ImportWindow* getImportWindow(GtkWidget *widget) {
      ImportWindow *importWindow = 0;
      std::map<GtkWindow*, ImportWindow*>::iterator it =
        importWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (importWindowMap.end() == it) {
        std::cout << "Cannot find ImportWindow window in the importWindowMap" << std::endl;
      } else {
        importWindow = it -> second;
      }
      return importWindow;
    }

    static void setImportWindow(GtkWidget *widget, ImportWindow *importWindow) {
      importWindowMap[GTK_WINDOW(gtk_widget_get_toplevel(widget))] = importWindow;
    }

    static void forgetImportWindow(GtkWidget *widget) {
      std::cout << "TODO setImportWindow WRITEME" << std::endl;
    }

    // QueryWindow registry methods

    static QueryWindow* getQueryWindow(GtkWidget *widget) {
      QueryWindow *queryWindow = 0;
      std::map<GtkWindow*, QueryWindow*>::iterator it =
        queryWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (queryWindowMap.end() == it) {
        std::cout << "Cannot find query window in the queryWindowMap" << std::endl;
      } else {
        queryWindow = it -> second;
      }
      return queryWindow;
    }

    static void setQueryWindow(GtkWidget *widget, QueryWindow *queryWindow) {
      queryWindowMap[GTK_WINDOW(gtk_widget_get_toplevel(widget))] = queryWindow;
    }

    static void forgetQueryWindow(GtkWidget *widget) {
      std::cout << "TODO forgetQueryWindow WRITEME" << std::endl;
    };

    // OpenProjectWindow registry methods

    static OpenProjectWindow* getOpenProjectWindow(GtkWidget *widget) {
      OpenProjectWindow *openProjectWindow = 0;
      std::map<GtkWindow*, OpenProjectWindow*>::iterator it =
        openProjectWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (openProjectWindowMap.end() == it) {
        std::cout << "Cannot find openProject window in the openProjectWindowMap" << std::endl;
      } else {
        openProjectWindow = it -> second;
      }
      return openProjectWindow;
    }

    static void setOpenProjectWindow(GtkWidget *widget, OpenProjectWindow *openProjectWindow) {
      openProjectWindowMap[GTK_WINDOW(gtk_widget_get_toplevel(widget))] = openProjectWindow;
    }

    static void forgetOpenProjectWindow(GtkWidget *widget) {
      std::cout << "TODO forgetOpenProjectWindow WRITEME" << std::endl;
    };

    // NewProjectWindow registry methods

    static NewProjectWindow* getNewProjectWindow(GtkWidget *widget) {
      NewProjectWindow *newProjectWindow = 0;
      std::map<GtkWindow*, NewProjectWindow*>::iterator it =
        newProjectWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (newProjectWindowMap.end() == it) {
        std::cout << "Cannot find newProject window in the newProjectWindowMap" << std::endl;
      } else {
        newProjectWindow = it -> second;
      }
      return newProjectWindow;
    }

    static void setNewProjectWindow(GtkWidget *widget, NewProjectWindow *newProjectWindow) {
      newProjectWindowMap[GTK_WINDOW(gtk_widget_get_toplevel(widget))] = newProjectWindow;
    }

    static void forgetNewProjectWindow(GtkWidget *widget) {
      std::cout << "TODO forgetNewProjectWindow WRITEME" << std::endl;
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
#endif // WINDOW_REGISTRY_H__


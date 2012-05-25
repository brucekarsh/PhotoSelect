#ifndef WINDOW_REGISTRY_H__
#define WINDOW_REGISTRY_H__

#include <iostream>
#include <gtk/gtk.h>
#include <map>
class PhotoSelectWindow;
class PreferencesWindow;
class ImportWindow;

/** Remembers a mapping from top-level windows to our objects so we can get find
**  our objects given a top-level window.
*/

class WindowRegistry {
    static std::map<GtkWindow*, PhotoSelectWindow*> photoSelectWindowMap;
    static std::map<GtkWindow*, PreferencesWindow*> preferencesWindowMap;
    static std::map<GtkWindow*, ImportWindow*> importWindowMap;
  public:

    // PhotoSelectWindow registry methods

    static PhotoSelectWindow* getPhotoSelectWindow(GtkWidget *widget) {
      PhotoSelectWindow *photoSelectWindow = 0;
      std::map<GtkWindow*, PhotoSelectWindow*>::iterator it =
        photoSelectWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (photoSelectWindowMap.end() == it) {
        printf("Cannot find PhotoSelect window in the photoSelectWindowMap\n");
      } else {
        photoSelectWindow = it -> second;
      }
      return photoSelectWindow;
    }

    static void setPhotoSelectWindow(GtkWidget *widget, PhotoSelectWindow *photoSelectWindow) {
      photoSelectWindowMap.insert(std::pair<GtkWindow*, PhotoSelectWindow*>(
          GTK_WINDOW(gtk_widget_get_toplevel(widget)), photoSelectWindow));
    }

    static void forgetPhotoSelectWindow(GtkWidget *widget) {
      std::cout << "XXX forgetPhotoSelectWindow WRITEME" << std::endl;
    };

    // PreferencesWindow registry methods

    static PreferencesWindow* getPreferencesWindow(GtkWidget *widget) {
      PreferencesWindow *preferencesWindow = 0;
      std::map<GtkWindow*, PreferencesWindow*>::iterator it =
        preferencesWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (preferencesWindowMap.end() == it) {
        printf("Cannot find PreferencesWindow window in the preferencesWindowMap\n");
      } else {
        preferencesWindow = it -> second;
      }
      return preferencesWindow;
    }

    static void setPreferencesWindow(GtkWidget *widget, PreferencesWindow *preferencesWindow) {
      preferencesWindowMap.insert(std::pair<GtkWindow*, PreferencesWindow*>(
          GTK_WINDOW(gtk_widget_get_toplevel(widget)), preferencesWindow));
    }

    static void forgetPreferencesWindow(GtkWidget *widget) {
      std::cout << "XXX setPreferencesWindow WRITEME" << std::endl;
    }

    // ImportWindow registry methods

    static ImportWindow* getImportWindow(GtkWidget *widget) {
      ImportWindow *importWindow = 0;
      std::map<GtkWindow*, ImportWindow*>::iterator it =
        importWindowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
      if (importWindowMap.end() == it) {
        printf("Cannot find ImportWindow window in the importWindowMap\n");
      } else {
        importWindow = it -> second;
      }
      return importWindow;
    }

    static void setImportWindow(GtkWidget *widget, ImportWindow *importWindow) {
      importWindowMap.insert(std::pair<GtkWindow*, ImportWindow*>(
          GTK_WINDOW(gtk_widget_get_toplevel(widget)), importWindow));
    }

    static void forgetImportWindow(GtkWidget *widget) {
      std::cout << "XXX setImportWindow WRITEME" << std::endl;
    }
    
    // Other registry methods

    static GtkWidget *get_toplevel_widget (GtkWidget *widget) {
      GtkWidget *parent;

      const int maxLevels = 10000; // Don't try forever
      int i;
      for (i=0; i<maxLevels; i++) {
        if (GTK_IS_MENU (widget)) {
          parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
        } else {
          parent = widget->parent;
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
};
#endif // WINDOW_REGISTRY_H__

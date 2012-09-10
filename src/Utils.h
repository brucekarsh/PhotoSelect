#ifndef UTILS_H__
#define UTILS_H__


class Utils {
  public:
    /** replacement for the deprecated gtk_widget_get_pointer */
    static void get_pointer(GtkWidget *widget, gint *pointer_x, gint *pointer_y) {
      GdkDeviceManager *device_manager =
          gdk_display_get_device_manager(gtk_widget_get_display (widget));
      GdkDevice *pointer = gdk_device_manager_get_client_pointer(device_manager);
      gdk_window_get_device_position(gtk_widget_get_window (widget),
          pointer, pointer_x, pointer_y, NULL);
    }
};

#endif // UTILS_H__

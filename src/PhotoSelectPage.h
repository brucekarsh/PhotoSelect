#ifndef PHOTOSELECTPAGE_H__
#define PHOTOSELECTPAGE_H__

#include "PageRegistry.h"
#include "Preferences.h"
#include <list>
#include <map>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PreferencesWindow.h"
#include "ImportWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include <boost/lexical_cast.hpp>

class PhotoFileCache;

class PhotoSelectPage {
  public:

    struct photo_tag_s {
      bool has_value;
      std::string value;
      bool value_is_null;
    };
  
    struct project_tag_s {
      bool has_value;
    };

    Preferences *thePreferences;
    int rotation;
    ConversionEngine conversionEngine;
    std::list<std::string> photoFilenameList;
    std::string project_name;
    sql::Connection *connection;
    PhotoFileCache *photoFileCache;

    GtkWidget *page_hbox;
    GtkWidget *page_vbox;
    GtkWidget *drawing_area;
    GtkWidget *button_hbox;
    GtkWidget *keep_button;
    GtkWidget *drop_button;
    GtkWidget *next_button;
    GtkWidget *back_button;
    GtkWidget *rotate_button;
    GtkWidget *gimp_button;
    GtkWidget *button_separator;
    GtkWidget *of_label;
    GtkWidget *filename_entry;
    GtkWidget *position_entry;
    GtkWidget *tab_label_hbox;
    GtkWidget *tab_label_label;
    GtkWidget *tab_label_button;
    GtkWidget *tag_view_box;
    float Dx, Dy; // displacement of the current image in screen coordinates
    float M;      // magnification of the current image (screen_size = m * image_size)
    bool drag_is_active;
    int drag_start_x;
    int drag_start_y;
    boolean calculated_initial_scaling;
    std::string tags_position;
    std::map<std::string, photo_tag_s> photo_tags;
    std::map<std::string, project_tag_s> project_tags;

    static const float ZOOMRATIO = 1.18920711500272106671;  // 2^(1/4)

  PhotoSelectPage(sql::Connection *connection_, PhotoFileCache *photoFileCache_) :
      conversionEngine(photoFileCache_), 
      rotation(0), drawing_area(0), thePreferences((Preferences*)0),
      connection(connection_), photoFileCache(photoFileCache_), M(1.0), Dx(0),
      Dy(0), drag_is_active(false), calculated_initial_scaling(false), tag_view_box(0),
      tags_position("right") {
  }

  PhotoSelectPage *clone() {
    PhotoSelectPage *cloned_photo_select_page = new PhotoSelectPage(connection, photoFileCache);
    cloned_photo_select_page->setup(photoFilenameList, project_name, thePreferences);
    cloned_photo_select_page->set_tags_position(tags_position);
    return cloned_photo_select_page;
  }

  GtkWidget *
  get_notebook_page() {
    return page_hbox;
  }

  GtkWidget *
  get_tab_label() {
    return tab_label_hbox;
  }

  void
  set_tags_position(const std::string position) {
    tags_position = position;
    add_tag_view();
  }

  void
  build_page() {
    // Make a label for the notebook tab
    tab_label_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    tab_label_label = gtk_label_new(project_name.c_str());
    tab_label_button = gtk_button_new();
    g_signal_connect(tab_label_button, "clicked", G_CALLBACK(tab_label_button_clicked_cb),
        (gpointer)this);
    gtk_button_set_relief(GTK_BUTTON(tab_label_button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click(GTK_BUTTON(tab_label_button), FALSE);
    gtk_widget_set_tooltip_text(GTK_WIDGET(tab_label_button), "Close page");
    GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
    gtk_button_set_image(GTK_BUTTON(tab_label_button), image);
    gtk_widget_set_size_request(tab_label_button, 0,0);
    GtkStyleContext *style_context = gtk_widget_get_style_context(GTK_WIDGET(tab_label_button));
    GtkCssProvider *css_provider = gtk_css_provider_new();
    std::string data = ".button {\n"
        "-GtkButton-default-border : 0px;\n"
        "-GtkButton-default-outside-border : 0px;\n"
        "-GtkButton-inner-border: 0px;\n"
        "-GtkWidget-focus-line-width : 0px;\n"
        "-GtkWidget-focus-padding : 0px;\n"
        "padding: 0px;\n"
        "}";
    gtk_css_provider_load_from_data(css_provider, data.c_str(), -1, NULL);
    gtk_style_context_add_provider(style_context,
        GTK_STYLE_PROVIDER(css_provider), 600); // GTK_STYLE_PROVIDER_PRIORITY_APPLICATION

    gtk_widget_show(tab_label_button);
    gtk_widget_show(tab_label_label);
    gtk_widget_show(tab_label_hbox);
    gtk_box_pack_start(GTK_BOX(tab_label_hbox), tab_label_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_label_hbox), tab_label_label, FALSE, FALSE, 0);

    // make a hbox to hold the page (page_hbox)
    page_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(page_hbox);

    // make a vbox to hold the page (page_vbox)
    page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(page_vbox);

    // add the page_vbox to the page_hbox
    gtk_box_pack_start(GTK_BOX(page_hbox), page_vbox, TRUE, TRUE, 0);

    // add a drawing area (drawing_area) to page_vbox
    drawing_area = gtk_drawing_area_new();
    gtk_widget_show(drawing_area);
    gtk_box_pack_start(GTK_BOX(page_vbox), drawing_area, TRUE, TRUE, 0);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(drawing_area_draw_cb), 0);
    g_signal_connect(drawing_area, "button-press-event",
        G_CALLBACK(drawing_area_button_press_cb), NULL);
    g_signal_connect(drawing_area, "button-release-event",
        G_CALLBACK(drawing_area_button_release_cb), NULL);
    g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(drawing_area_scroll_cb), NULL);
    g_signal_connect(drawing_area, "motion-notify-event",
        G_CALLBACK(drawing_area_motion_notify_cb), NULL);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
        | GDK_SCROLL_MASK | GDK_BUTTON_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

    // make an hbox (button_hbox) to hold the buttons, etc and add it to page_vbox
    button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(button_hbox);
    gtk_box_pack_start(GTK_BOX(page_vbox), button_hbox, FALSE, FALSE, 0);

    // add Keep, Drop, Next, Back, Rotate and Gimp buttons to button_hbox
    keep_button = gtk_button_new_with_label("Keep");
    gtk_widget_show(keep_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), keep_button, FALSE, FALSE, 0);
    g_signal_connect(keep_button, "clicked", G_CALLBACK(keep_button_clicked_cb), 0);
    
    drop_button = gtk_button_new_with_label("Drop");
    gtk_widget_show(drop_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), drop_button, FALSE, FALSE, 0);
    g_signal_connect(drop_button, "clicked", G_CALLBACK(drop_button_clicked_cb), 0);
    
    next_button = gtk_button_new_with_label("Next");
    gtk_widget_show(next_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), next_button, FALSE, FALSE, 0);
    g_signal_connect(next_button, "clicked", G_CALLBACK(next_button_clicked_cb), 0);
    
    back_button = gtk_button_new_with_label("Back");
    gtk_widget_show(back_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), back_button, FALSE, FALSE, 0);
    g_signal_connect(back_button, "clicked", G_CALLBACK(back_button_clicked_cb), 0);
    
    rotate_button = gtk_button_new_with_label("Rotate");
    gtk_widget_show(rotate_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), rotate_button, FALSE, FALSE, 0);
    g_signal_connect(rotate_button, "clicked", G_CALLBACK(rotate_button_clicked_cb), 0);
    
    gimp_button = gtk_button_new_with_label("Gimp");
    gtk_widget_show(gimp_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), gimp_button, FALSE, FALSE, 0);
    g_signal_connect(gimp_button, "clicked", G_CALLBACK(gimp_button_clicked_cb), 0);

    // add a text entry (filename_entry) to button_hbox   
    filename_entry = gtk_entry_new();
    gtk_entry_set_alignment(GTK_ENTRY(filename_entry), 0.5);
    gtk_box_pack_start(GTK_BOX(button_hbox), filename_entry, TRUE, TRUE, 3);
    gtk_entry_set_text(GTK_ENTRY(filename_entry), "filename");
    GValue value = {0};
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, false);
    g_object_set_property(G_OBJECT(filename_entry),"editable", &value);
    g_object_set_property(G_OBJECT(filename_entry),"has-frame", &value);
    gtk_widget_show(filename_entry);
    
    // add a separator (button_separator) to button_hbox
    button_separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_show(button_separator);
    gtk_box_pack_end(GTK_BOX(button_hbox), button_separator, FALSE, FALSE, 0);

    // add a label (of_label) to button_hbox
    of_label = gtk_label_new("of -1");
    gtk_widget_show(of_label);
    gtk_box_pack_end(GTK_BOX(button_hbox), of_label, FALSE, FALSE, 0);
    
    // add a text entry (position_entry) to button_hbox   
    position_entry = gtk_entry_new();
    gtk_entry_set_alignment(GTK_ENTRY(position_entry), 1.0);
    gtk_entry_set_width_chars(GTK_ENTRY(position_entry), 10);
    gtk_widget_show(position_entry);
    gtk_box_pack_end(GTK_BOX(button_hbox), position_entry, FALSE, FALSE, 0);
    add_tag_view();
    g_signal_connect(position_entry, "activate", G_CALLBACK(position_entry_activate_cb), 0);
  }

  std::map<std::string, photo_tag_s> get_photo_tags() {
    std::map<std::string, photo_tag_s> tags;
    std::string sql = "SELECT Tag.name, ProjectTag.hasValue, TagChecksumValue.value FROM Tag "
        "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
        "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
        "INNER JOIN TagChecksum ON (TagChecksum.tagId = Tag.id) "
        "INNER JOIN PhotoFile ON (TagChecksum.checksumId = PhotoFile.checksumId) "
        "LEFT JOIN TagChecksumValue ON (TagChecksumValue.tagId = Tag.id "
        "AND TagChecksumValue.checksumId = PhotoFile.checksumId) "
        "WHERE (Project.name = ? and PhotoFile.filePath = ?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, project_name);
    std::string file_name = conversionEngine.getPhotoFilePath();
    prepared_statement->setString(2, file_name);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while (rs->next()) {
      photo_tag_s tag;
      std::string name = rs->getString(1);
      tag.has_value = rs->getBoolean(2);
      tag.value = rs->getString(3);
      tag.value_is_null = rs->wasNull();
      tags[name] = tag;
    }
    return tags;
  }

  std::map<std::string, project_tag_s> get_project_tags() {
    std::map<std::string, project_tag_s> tags;
    std::string sql = "SELECT Tag.name, ProjectTag.hasValue FROM Tag "
        "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
        "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
        "WHERE (Project.name = ?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, project_name);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while (rs->next()) {
      project_tag_s tag;
      std::string name = rs->getString(1);
      bool has_value = rs->getBoolean(2);
      tag.has_value = has_value;
      tags[name]=tag;
    }
    return tags;
  }

  // Adds a tag view to the PhotoSelectPage. The tag view (tag_view_box) is put into
  // either page_hbox or page_vbox, depending on the tags position (from the view/tags
  // menubar menu.
  // Additionally, it sets up a map (photo_tags) of the tags for the current photo and a
  // list (project_tags) of tags for the current project.
  void add_tag_view() {
    GtkWidget *tag_view_scrolled_window = NULL;
    GtkWidget *tag_view_tags_box = NULL;

    // Destroy any existing tag_view_box
    if (NULL != tag_view_box) {
      gtk_widget_destroy(tag_view_box);
      tag_view_box = NULL;
    }

    // Get all the tags for this photo
    photo_tags = get_photo_tags();

    // Get all the tags for this project
    project_tags = get_project_tags();

    // Don't do anything if the tag view is turned off
    if (tags_position == "none") {
      return;
    }
    
    // Make a box (tag_view_box) for the tag_view
    tag_view_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(tag_view_box);

    // Put a label (tag_view_label) into the tag_view_box
    GtkWidget *tag_view_label = gtk_label_new("Tags");
    gtk_widget_show(tag_view_label);
    gtk_box_pack_start(GTK_BOX(tag_view_box), tag_view_label, FALSE, FALSE, 0);
    

    // Make a scrolled window (tag_view_scrolled_window) to scroll the tag list
    tag_view_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tag_view_scrolled_window),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(tag_view_scrolled_window);

    // Make a box (tag_view_tags_box) to go into the scrolled window
    tag_view_tags_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(tag_view_tags_box);

    // Put check buttons in tag_view_tags_box, one for each tag in the project
    typedef std::pair<std::string, project_tag_s> map_entry_t;
    BOOST_FOREACH(map_entry_t map_entry, project_tags) {
      std::string name = map_entry.first;
      project_tag_s project_tag = map_entry.second;
      // Make a button, pack it, show it and connect it.
      GtkWidget *button = gtk_check_button_new_with_label(name.c_str());
      gtk_box_pack_start(GTK_BOX(tag_view_tags_box), button, FALSE, FALSE, 0);
      gtk_widget_show(button);

      // If this tag is supposed to have a value, make an entry, pack it and show it
      GtkWidget *entry = NULL;
      if (true == project_tag.has_value) {
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(tag_view_tags_box), entry, FALSE, FALSE, 0);
      }

      // If the tag is set for this photo, activate its check button.
      if (1 == photo_tags.count(name)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
      }

      // If this tag is supposed to have a value but the tag is not set for this photo,
      // make its entry non-sensitive.
      if (NULL != entry && 1 != photo_tags.count(name)) {
          gtk_widget_set_sensitive(entry, false);
      }
        
      // If this tag is supposed to have a value and the tag is set and it actually has
      // a value, set the entry to its value. 

      if (NULL != entry && 1 == photo_tags.count(name)) {
        photo_tag_s photo_tag = photo_tags[name];
        if (false == photo_tag.value_is_null) {
          gtk_entry_set_text(GTK_ENTRY(entry), photo_tag.value.c_str());
        }
        g_signal_connect(GTK_EDITABLE(entry), "changed", G_CALLBACK(tag_entry_changed_cb),
            (gpointer)button);
      }
      g_signal_connect(button, "toggled", G_CALLBACK(tag_button_clicked_cb), (gpointer)entry);
    }

    // Put the tag_view_scrolled_window into the tag_view_box.
    gtk_box_pack_start(GTK_BOX(tag_view_box), tag_view_scrolled_window, TRUE, TRUE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tag_view_scrolled_window),
        tag_view_tags_box);

    // Put the tag_view_box into the page_hbox.
    if (tags_position == "left") {
      gtk_box_pack_start(GTK_BOX(page_hbox), tag_view_box, FALSE, FALSE, 0);
      gtk_box_reorder_child(GTK_BOX(page_hbox), tag_view_box, 0);
    } else if (tags_position == "right") {
      gtk_box_pack_start(GTK_BOX(page_hbox), tag_view_box, FALSE, FALSE, 0);
    } else if (tags_position == "top") {
      gtk_box_pack_start(GTK_BOX(page_vbox), tag_view_box, FALSE, FALSE, 0);
      gtk_box_reorder_child(GTK_BOX(page_vbox), tag_view_box, 0);
    } else if (tags_position == "bottom") {
      gtk_box_pack_start(GTK_BOX(page_vbox), tag_view_box, FALSE, FALSE, 0);
    }
  }

  void setup(std::list<std::string> photoFilenameList_, std::string project_name_,
      Preferences *thePreferences) {
    this->thePreferences = thePreferences;
    this->project_name = project_name_;
    photoFilenameList = photoFilenameList_;

    // Set up a conversion engine.
    conversionEngine.setPhotoFileList(&photoFilenameList);

    // Build the page
    build_page();

    // Add it to the registry so we can find this object when we get a callback
    PageRegistry<PhotoSelectPage>::setPage(page_hbox, this);

    // Set up the of_label
    char ofstring[20];
    int numberOfPhotos = photoFilenameList.size();
    snprintf(ofstring, 20, "of %d      ", numberOfPhotos);
    ofstring[19]=0;
    gtk_label_set_text(GTK_LABEL(of_label), ofstring);

    // Set up the position_entry
    set_position_entry();
  } 

  void
  calculate_initial_scaling() {
    calculated_initial_scaling = true;
    ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile();
    if (NULL == convertedPhotoFile) {
      Dx = 0.0;
      Dy = 0.0;
      M = 1.0;
      return;
    }

    gint surface_height = gtk_widget_get_allocated_height(drawing_area);
    gint surface_width = gtk_widget_get_allocated_width(drawing_area);

    int width = convertedPhotoFile->width;
    int height = convertedPhotoFile->height;

    if (rotation == 0 || rotation == 2) {
      M = MIN(
          (double) surface_width / width,
          (double) surface_height /height);
    } else {
      M = MIN(
          (double) surface_width / height,
          (double) surface_height / width);
    }

    // Don't allow zero magnification. It would occur if surface_width or surface_height
    // were zero.
    if (M == 0.0) {
      M = 1.0;
    }
    Dx = 0.0;
    Dy = 0.0;
  }

  /** replacement for the deprecated gtk_widget_get_pointer */
  void get_pointer(GtkWidget *widget, gint *pointer_x, gint *pointer_y) {
    GdkDeviceManager *device_manager =
        gdk_display_get_device_manager(gtk_widget_get_display (widget));
    GdkDevice *pointer = gdk_device_manager_get_client_pointer(device_manager);
    gdk_window_get_device_position(gtk_widget_get_window (widget),
        pointer, pointer_x, pointer_y, NULL);
  }

  void zoom(float zoomfactor)
  {
    int sx, sy;
    //gtk_widget_get_pointer(drawing_area, &sx, &sy);
    get_pointer(drawing_area, &sx, &sy);
    gint surface_height = gtk_widget_get_allocated_height(drawing_area);
    gint surface_width = gtk_widget_get_allocated_width(drawing_area);

    if (sx >= 0 && sx < surface_width &&
        sy >= 0 && sy < surface_height) {

      float sx0 = sx - surface_width/2.0;
      float sy0 = sy - surface_height/2.0;

      float px = (sx0 - Dx) / M;
      float py = (sy0 - Dy) / M;

      M *= zoomfactor;

      Dx = sx0 - M * px;
      Dy = sy0 - M * py;
    } else {
      std::cout << "ineligible scroll position" << std::endl;
    }
  }


  void
  position_entry_activate() {
    std::string valstr = gtk_entry_get_text(GTK_ENTRY(position_entry));
    int val = atoi(valstr.c_str());
    if (val < 1) {
      val = 1;
    }
    int siz = photoFilenameList.size();
    if (val > siz) {
      val = siz;
    }
    conversionEngine.go_to(val-1);   
    rotation = 0;
    calculate_initial_scaling();
    set_position_entry();
  }

  void
  drawing_area_button_press(GdkEvent *event) {
    int button = event->button.button;
    if (button == 1) {
      drag_start_x = event->button.x;
      drag_start_y = event->button.y;
      drag_is_active = true;
    }
  }

  void
  drawing_area_button_release(GdkEvent *event) {
    int button = event->button.button;
    if (button == 1 && drag_is_active) {
      int drag_end_x = event->button.x;
      int drag_end_y = event->button.y;
      int deltaX = drag_end_x - drag_start_x;
      int deltaY = drag_end_y - drag_start_y;
      Dx += deltaX;
      Dy += deltaY;
      redraw_image();
      drag_is_active = false;
    }
  }

  void
  drawing_area_scroll(GdkEvent *event) {
    if (event->scroll.direction == GDK_SCROLL_UP) {
      zoom(ZOOMRATIO);
    } else if (event->scroll.direction == GDK_SCROLL_DOWN) {
      zoom(1.0/ZOOMRATIO);
    } else {
      std::cout << "don't know which way to scroll" << std::endl;
    }
  }

  void
  drawing_area_motion_notify(GdkEvent * event) {
    if (drag_is_active) {
        int drag_end_x = event->motion.x;
        int drag_end_y = event->motion.y;
        int deltaX = drag_end_x - drag_start_x;
        int deltaY = drag_end_y - drag_start_y;
        Dx += deltaX;
        Dy += deltaY;
        drag_start_x = drag_end_x;
        drag_start_y = drag_end_y;
        redraw_image();
    }
  }

  /** Sets to position_entry widget to reflect the positioon of the ConversionEngine.
      The ConversionEngine counts 0..N-1 and the position_entry counts 1..N. */
  void
  set_position_entry() {
    int val = conversionEngine.get_position();
    std::string valstring =  boost::lexical_cast<std::string>(val + 1);
    gtk_entry_set_text(GTK_ENTRY(position_entry), valstring.c_str());
  }

  void redraw_image() {
    ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(); 
    std::string photoFilePath = conversionEngine.getPhotoFilePath();
    gtk_entry_set_text(GTK_ENTRY(filename_entry), photoFilePath.c_str());
    gtk_editable_set_position(GTK_EDITABLE(filename_entry), photoFilePath.size());
    
    if (!calculated_initial_scaling) {
      calculate_initial_scaling();
    }
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(drawing_area));
    if (NULL == convertedPhotoFile) {
      return;
    }
    gint surface_height = gtk_widget_get_allocated_height(drawing_area);
    gint surface_width = gtk_widget_get_allocated_width(drawing_area);

    unsigned char *transformed_image = convertedPhotoFile->scale_and_pan_and_rotate(
      surface_width, surface_height, M, Dx, Dy, (float)rotation);

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, surface_width);
    cairo_surface_t *source_surface = cairo_image_surface_create_for_data(
        transformed_image,
        CAIRO_FORMAT_RGB24, surface_width, surface_height, stride);
    cairo_set_source_surface(cr, source_surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(source_surface);

    free(transformed_image);
  }

  void keep() {
    // TODO WRITEME or DELETEME
  }

  void drop() {
    // TODO WRITEME or DELETEME
  }

  void next() {
    rotation = 0;
    conversionEngine.next();   
    calculate_initial_scaling();
    set_position_entry();
  }

  void back() {
    rotation = 0;
    conversionEngine.back();   
    calculate_initial_scaling();
    set_position_entry();
  }

  void rotate() {
    rotation += 1;
    if (rotation == 4) {
      rotation = 0;
    }
  }

  void gimp() {
    // TODO WRITEME or DELETEME
  }

  static void tab_label_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = (PhotoSelectPage *)data;
    if (0 != photoSelectPage) {
      photoSelectPage->quit();
    }
  }

  static void keep_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> keep();
      photoSelectPage->add_tag_view();
      photoSelectPage -> redraw_image();
    }
  }

  static void drop_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> drop();
      photoSelectPage->add_tag_view();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  next_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> next();
      photoSelectPage->add_tag_view();
      photoSelectPage->redraw_image();
    }
  }

  static void
  back_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> back();
      photoSelectPage->add_tag_view();
      photoSelectPage->redraw_image();
    }
  }

  static void
  rotate_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> rotate();
      photoSelectPage->redraw_image();
    }
  }

  static void
  gimp_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> gimp();
      photoSelectPage -> redraw_image();
    }
  }

  void quit();

  static void
  drawing_area_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> redraw_image();
    }
  }

  static void
  position_entry_activate_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->position_entry_activate();
      photoSelectPage->add_tag_view();
      photoSelectPage -> redraw_image();
    }
  }
  static void
  drawing_area_button_press_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_button_press(event);
    }
  }
  static void
  drawing_area_button_release_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_button_release(event);
    }
  }
  static void
  drawing_area_scroll_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_scroll(event);
      photoSelectPage -> redraw_image();
    }
  }
  static void
  drawing_area_motion_notify_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_motion_notify(event);
    }
  }

  static void
  tag_button_clicked_cb(GtkToggleButton *togglebutton, gpointer user_data) {
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(
        GTK_WIDGET(togglebutton));
    if (0 != photoSelectPage) {
      photoSelectPage->tag_button_clicked(togglebutton, user_data);
    }
  }

  void
  tag_button_clicked(GtkToggleButton *togglebutton, gpointer user_data) {
    std::string tag_name = gtk_button_get_label(GTK_BUTTON(togglebutton));
    bool active = gtk_toggle_button_get_active(togglebutton);
    std::string file_name = conversionEngine.getPhotoFilePath();
    project_tags = get_project_tags();
    photo_tags = get_photo_tags();
    GtkWidget *entry = NULL;
    if (0 != user_data) {
      entry = GTK_WIDGET(user_data);
    }

    // Ignore this click if it's for a tag that's not in our project
    if (0 == project_tags.count(tag_name)) {
      return;
    }

    if (active) {
      add_tag(tag_name, file_name);
      if (NULL != entry) {
        gtk_entry_set_text(GTK_ENTRY(entry), photo_tags[tag_name].value.c_str());
        gtk_widget_set_sensitive(entry, true);
      }
    } else {
      remove_tag(tag_name, file_name);
      if (NULL != entry) {
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        gtk_widget_set_sensitive(entry, false);
      }
    }
  }

  void
  add_tag(std::string tag_name, std::string file_name) {
    std::string sql = "INSERT INTO TagChecksum (tagId, checksumId) "
        "SELECT DISTINCT Tag.id as tagId, Checksum.id as checksumId "
        "FROM Tag, Checksum, PhotoFile "
        "WHERE Tag.name = ? AND PhotoFile.filePath = ? AND Checksum.id = PhotoFile.checksumId";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, tag_name);
    prepared_statement->setString(2, file_name);
    prepared_statement->execute();
    connection->commit();
  }

  void
  add_value(std::string tag_name, std::string tag_value, std::string photoFilePath) {
    std::string sql =
        "INSERT INTO TagChecksumValue (tagId, checksumId, value) "
        "SELECT Tag.id, Checksum.id, ? FROM  PhotoFile "
        "INNER JOIN Checksum ON (PhotoFile.ChecksumId = Checksum.id) "
        "INNER JOIN TagChecksum ON (TagChecksum.checksumId = Checksum.id) "
        "INNER JOIN Tag ON (TagChecksum.tagId = Tag.id) "
        "WHERE Tag.name=? "
        "AND PhotoFile.filePath=? "
        "ON DUPLICATE KEY UPDATE TagChecksumValue.value = ?";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, tag_value);
    prepared_statement->setString(2, tag_name);
    prepared_statement->setString(3, photoFilePath);
    prepared_statement->setString(4, tag_value);
    prepared_statement->execute();
    connection->commit();
  }

  void
  remove_tag(std::string tag_name, std::string file_name) {
    std::string sql = "DELETE FROM TagChecksum "
        "USING Tag, Checksum, PhotoFile, TagChecksum "
        "WHERE Tag.name = ? AND PhotoFile.filePath = ? "
        "AND Checksum.id = PhotoFile.checksumId AND TagChecksum.checksumId=Checksum.id "
        "AND TagChecksum.tagId=Tag.id";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, tag_name);
    prepared_statement->setString(2, file_name);
    prepared_statement->execute();
    connection->commit();
  }

  static void
  tag_entry_changed_cb(GtkEditable *editable, gpointer user_data) {
    gchar *chars = gtk_editable_get_chars(editable, 0, -1);
    std::string tag_value = chars;
    free(chars);
    GtkWidget* button = GTK_WIDGET(user_data);
    std::string tag_name = gtk_button_get_label(GTK_BUTTON(button));
    
    PhotoSelectPage *photoSelectPage = PageRegistry<PhotoSelectPage>::getPage(GTK_WIDGET(editable));
    if (NULL != photoSelectPage) {
      photoSelectPage->tag_entry_changed(tag_name, tag_value);
    }
  }

  void tag_entry_changed(std::string tag_name, std::string tag_value) {
    std::string photoFilePath = conversionEngine.getPhotoFilePath();
    add_value(tag_name, tag_value, photoFilePath);
  }
};

#include "BaseWindow.h"

  inline void PhotoSelectPage::quit() {
    BaseWindow *baseWindow = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(drawing_area));
    if (NULL != baseWindow) {
      baseWindow->remove_page(page_hbox);
    }
  }
#endif  // PHOTOSELECTPAGE_H__

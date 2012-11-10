#ifndef SINGLEPHOTOPAGE_H__
#define SINGLEPHOTOPAGE_H__

#include <list>
#include <map>
#include <gtk/gtk.h>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

#include "Db.h"
#include "ConversionEngine.h"
#include "PhotoSelectPage.h"

class PhotoFileCache;
class Preferences;

class SinglePhotoPage : public PhotoSelectPage {
  public:

    Preferences *thePreferences;
    int rotation;
    ConversionEngine conversionEngine;
    std::vector<std::string> photoFilenameVector;
    std::vector<std::string> adjusted_date_time_vector;
    std::string project_name;
    PhotoFileCache *photoFileCache;

    GtkWidget *page_hbox;
    GtkWidget *page_vbox;
    GtkWidget *page_left_vbox;
    GtkWidget *page_right_vbox;
    GtkWidget *drawing_area;
    GtkWidget *button_hbox;
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
    GtkWidget *exif_view_box;
    float Dx, Dy; // displacement of the current image in screen coordinates
    float M;      // magnification of the current image (screen_size = m * image_size)
    bool drag_is_active;
    int drag_start_x;
    int drag_start_y;
    bool calculated_initial_scaling;
    std::string tags_position;
    std::string exifs_position;
    std::set<std::string> photo_tags;
    std::map<std::string, Db::project_tag_s> project_tags;

  SinglePhotoPage(PhotoFileCache *photoFileCache_);
  ~SinglePhotoPage();
  void load_extra_menu_items();
  const std::string &get_project_name();
  void edit_unselect_all_activate();
  PhotoSelectPage *clone();
  GtkWidget * get_notebook_page();
  GtkWidget * get_tab_label();
  void set_tags_position(const std::string position);
  void set_exifs_position(const std::string position);
  void build_page();
  gboolean drawing_area_key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! grab the focus when the GtkDrawingArea is entered. This lets it get keyboard events.
  //! The grab is removed in drawing_area_leave()
  gboolean drawing_area_enter(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! un-grab the focus when the GtkDrawingArea is left. This lets it get keyboard events.
  //! Focus is  grabbed in drawing_area_enter()
  gboolean drawing_area_leave(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  // Adds a tag view to the SinglePhotoPage. The tag view (tag_view_box) is put into
  // either page_hbox or page_vbox, depending on the tags position (from the view/tags
  // menubar menu.
  // Additionally, it sets up a set (photo_tags) of the tags for the current photo and a
  // list (project_tags) of tags for the current project.
  void rebuild_tag_view();
  void rebuild_exif_view();
  std::string shorten_exif_name(const std::string &exif_name);
  bool contains(std::list<std::string> search_list, std::string search_value);
  std::map<std::string, std::string> get_exifs();

  //!
  //! Returns a value for an attribute value from a json node map given an attribute name
  char *
  get_value_by_name(xercesc::DOMNamedNodeMap *attributes, std::string exif_name);

  // Used only for debugging
  void print_node(xercesc::DOMNode *node);

  // Used only for debugging
  std::string node_type_string(xercesc::DOMNode::NodeType nt);

  //!
  //! Parse a json string into a DOM using Xerces
  bool parse_json_string(xercesc::XercesDOMParser* parser, std::string json_string);
  void setup(std::vector<std::string> photoFilenameVector_,
      std::vector<std::string> adjusted_date_time_vector_, std::string project_name_,
      Preferences *thePreferences) ;
  void calculate_initial_scaling();
  void zoom(float zoomfactor);
  void position_entry_activate();
  void set_position(int val);
  gboolean drawing_area_button_press(GdkEvent *event);
  gboolean drawing_area_button_release(GdkEvent *event);
  gboolean drawing_area_scroll(GdkEvent *event);
  void drawing_area_motion_notify(GdkEvent * event);

  /** Sets to position_entry widget to reflect the positioon of the ConversionEngine.
      The ConversionEngine counts 0..N-1 and the position_entry counts 1..N. */
  void set_position_entry();
  void invalidate_image();
  void redraw_image(); 
  void next();
  void back();
  void rotate();
  void gimp();
  void quit();
  void add_page_to_base_window(PhotoSelectPage *photo_page);
  void open_multi_photo_page(int index);
  void tag_button_clicked(GtkToggleButton *togglebutton, gpointer user_data);

  // Static member functions

  static gboolean drawing_area_key_press_cb(
      GtkWidget *widget, GdkEvent *event, gpointer user_data);
  static gboolean drawing_area_enter_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  static gboolean drawing_area_leave_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  static void tab_label_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void next_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void back_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void rotate_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void gimp_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void drawing_area_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
  static void position_entry_activate_cb(GtkWidget *widget, gpointer data);
  static gboolean drawing_area_button_press_cb(GtkWidget *widget, GdkEvent *event, gpointer data);
  static gboolean drawing_area_button_release_cb(
      GtkWidget *widget, GdkEvent *event, gpointer data);
  static gboolean drawing_area_scroll_cb(GtkWidget *widget, GdkEvent *event, gpointer data);
  static void drawing_area_motion_notify_cb(GtkWidget *widget, GdkEvent *event, gpointer data);
  static void tag_button_clicked_cb(GtkToggleButton *togglebutton, gpointer user_data);
};
#endif  // SINGLEPHOTOPAGE_H__

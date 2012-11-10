#ifndef MULTIPHOTOPAGE_H__
#define MULTIPHOTOPAGE_H__

#include "PhotoSelectPage.h"
#include <list>
#include <map>
#include "ConversionEngine.h"
#include <boost/thread.hpp>
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
#include "WorkList.h"
#include "TicketRegistry.h"
#include "StockThumbnails.h"

extern WorkList work_list;
extern StockThumbnails *stock_thumbnails;
extern TicketRegistry ticket_registry;

class PhotoFileCache;
class Preferences;
class SinglePhotoPage;
  
class MultiPhotoPage : public PhotoSelectPage {
  public:

    // This enum is used by GtkListStore
    enum {
      COL_PIXBUF,
      COL_MARKUP,
      NUM_COLS
    };

    // This struct represents entries in the pixbuf_map. The pixbuf_map is a map keyed by
    // the index (sequence number on the page of a thumbnail). Each entry is a thumbnail
    // passed in asynchronously be a Worker. They are rendered and removed by an idle callback.
    struct PixbufMapEntry {
      PixbufMapEntry(GdkPixbuf *pixbuf = NULL, int rotation = 0, long priority = 0)
          : pixbuf(pixbuf), rotation(rotation), priority(priority) {};
      GdkPixbuf *pixbuf;
      int rotation;
      long priority;
    };

    //! Holds the state of a single photo on a MultiPhotoPage
    class PhotoState {
        int index;
        bool is_selected;
        int rotation;
        GdkPixbuf *pixbuf;
      public:
        PhotoState(bool is_selected = false, int index = 0) :
            is_selected(is_selected), index(index), pixbuf(NULL)
            {};
        ~PhotoState() { clear_pixbuf(); };
        int get_index() { return index; };
        int get_is_selected() { return is_selected; };
        void set_is_selected(bool b) { is_selected = b; };
        int get_rotation() { return rotation; };
        GdkPixbuf* get_pixbuf() { return pixbuf; };
        void set_pixbuf(GdkPixbuf *pixbuf, int rotation) {
          clear_pixbuf();
	  this->pixbuf = pixbuf;
          g_object_ref(G_OBJECT(pixbuf));
          this->rotation = rotation;
        }
        void clear_pixbuf() {
          if (pixbuf) {
            g_object_unref(G_OBJECT(pixbuf));
            pixbuf = 0;
          }
          rotation = 0;
        }
    };

    static const int ICON_WIDTH = 380;
    static const int ICON_HEIGHT = 285;
    static const int ICON_STRIDE = ICON_WIDTH * 3;

    Preferences *thePreferences;
    ConversionEngine conversionEngine;
    std::vector<std::string> photoFilenameVector;
    std::vector<std::string> adjusted_date_time_vector;
    std::map<int, PhotoState> photo_state_map;
    std::map<int, PixbufMapEntry> pixbuf_map;
    std::string project_name;
    PhotoFileCache *photoFileCache;
    GtkListStore *list_store;
    GtkTreeModel *tree_model_filter;
    int current_index;
    boost::mutex class_mutex;
    long ticket_number; // TicketRegistry ticket number
    gint idle_id;


    GtkWidget *page_hbox;
    GtkWidget *page_vbox;
    GtkWidget *page_left_vbox;
    GtkWidget *page_right_vbox;
    GtkWidget *tags_left_hbox;
    GtkWidget *tags_right_hbox;
    GtkWidget *exif_left_hbox;
    GtkWidget *exif_right_hbox;
    GtkWidget *central_hbox;
    GtkWidget *tab_label_hbox;
    GtkWidget *tab_label_label;
    GtkWidget *tab_label_button;
    GtkWidget *tag_view_box;
    GtkWidget *exif_view_box;
    GtkWidget *icon_view;
    GtkWidget *scrolled_window;
    GtkWidget *show_all_menu_item;
    GtkWidget *show_these_tags_menu_item;
    GtkWidget *dont_show_these_tags_menu_item;
    std::string tags_position;
    std::string exifs_position;
    std::set<std::string> photo_tags;
    std::map<std::string, Db::project_tag_s> project_tags;
    std::map<std::string, int> all_tag_counts;
    std::map<std::string, int> set_tag_counts;
    std::map<std::string, int> clear_tag_counts;
    std::map<GtkWidget *, std::string> tag_button_map;
    // all_photo_tags_for_project[file_name][tag_name]
    std::map<std::string, std::set<std::string> > all_photo_tags_for_project;
    GdkPixbuf *gtk_stock_missing_image;
    bool view_filter_show_all;
    bool view_filter_show;
    bool view_filter_dont_show;
    std::list<GtkWidget *> view_filter_show_menu_items;
    std::list<GtkWidget *> view_filter_dont_show_menu_items;
    std::set<std::string> view_filter_show_tags;
    std::set<std::string> view_filter_dont_show_tags;


  MultiPhotoPage(PhotoFileCache *photoFileCache_);
  ~MultiPhotoPage();
  const std::string &get_project_name();
  virtual void rotate(int index);
  void announce_row_change(int index);
  void edit_unselect_all_activate();
  void select_thumbnail_by_index(int index, bool new_state);
  PhotoSelectPage *clone();
  GtkWidget * get_notebook_page();
  GtkWidget * get_tab_label();
  void set_tags_position(const std::string position);
  void set_exifs_position(const std::string position);
  void build_page();
  void scroll_view_value_changed(GtkAdjustment *adjustment, gpointer user_data);
  gboolean idle();

  //! Puts a thumbnail in the pixbuf_map. Thumbnails are then rendered via the idle callback.
  //! We don't render them asynchronously from the worker thread because that causes too much
  //! flashing.
  bool set_thumbnail(int index, GdkPixbuf *pixbuf, int rotation, long priority);

  //! Puts a thumbnail into the photo_state and into the list_store
  //! \param index the index of the thumbnail in the (unfiltered) list_store.
  //! \param pixbuf the pixbuf to put into the photo_state and list_store
  //! \param rotation rotation of the put copy [0= unrotated, 1=90, 2=180, 3=270. counterclockwise]
  void apply_thumbnail(int index, GdkPixbuf *pixbuf, int rotation);

  // Adds a tag view to the MultiPhotoPage. The tag view (tag_view_box) is put into
  // either page_hbox or page_vbox, depending on the tags position (from the view/tags
  // menubar menu.
  // Additionally, it sets up a map (photo_tags) of the tags for the current photo and a
  // list (project_tags) of tags for the current project.
  void rebuild_tag_view();
  void count_tags();
  void rebuild_exif_view();
  std::string shorten_exif_name(const std::string &exif_name);
  bool contains(std::list<std::string> search_list, std::string search_value);
  
  //! Get the exifs from the database and return them as a map from
  //! the exif name to the exif value
  std::map<std::string, std::string> get_exifs();

  //!
  //! Returns a value for an attribute value from a json node map given an attribute name
  char * get_value_by_name(xercesc::DOMNamedNodeMap *attributes, std::string exif_name);

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
  void get_photo_thumbnail(PhotoState &photo_state, int surface_width, int surface_height);
  int tdiff(const struct timespec &endtime, const struct timespec &starttime);
  void calculate_scaling(double &M, int image_width, int image_height,
      int surface_width, int surface_height) const;
  void clear_button_clicked(GtkWidget *widget, gpointer data);
  void set_button_clicked(GtkWidget *widget, gpointer data);

  //! non-static callback for a keyboard key press on the GtkIconView icon_view.
  //! \param widget icon_view
  //! \param GdkEvent the event associated with the key press
  //! \param user_data NULL
  gboolean icon_view_key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  void set_position(int val);

  //! Object callback for a mouse button press
  gboolean icon_view_button_press(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! Object callback for a LEFT mouse button press
  gboolean icon_view_button_press_left(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! Object callback for a RIGHT mouse button press
  gboolean icon_view_button_press_right(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! Find the index (index of photo in the photoFileVector) of the icon under
  //! the cursor in a GtkIconView. Returns -1 if the cursor is not over an icon.
  int find_photo_index(GtkWidget *widget);
  gboolean icon_view_popup_menu(GtkWidget *widget, gpointer user_data);
  void view_icon_view_popup_menu(GtkWidget *widget, GdkEventButton *event, gpointer userdata);

  //! Implements the Extend selection to here popup menu entry
  //! finds the index of the thumbnail where the popup was activated, then searches
  //! backwards for the first visible selected thumbnail. Then selects all the visible
  //! thumbnails in that range.
  void icon_view_popup_extend_selection_activate(GtkMenuItem *menu_item, gpointer user_data);
  bool is_visible_thumbnail(int index);
  void icon_view_popup_open_image_viewer_activate(GtkMenuItem *menu_item, gpointer user_data);
  void icon_view_popup_tag_toggled(GtkMenuItem *menu_item, gpointer user_data);

  //! grab the focus when the GtkIconView is entered. This lets it get keyboard events.
  //! The grab is removed in icon_view_leave()
  gboolean icon_view_enter(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! un-grab the focus when the GtkIconView is left. This lets it get keyboard events.
  //! Focus is  grabbed in icon_view_enter()
  gboolean icon_view_leave(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  void load_extra_menu_items();
  void quit();
  void add_page_to_base_window(PhotoSelectPage *photo_page);
  void open_single_photo_page(int index);
  std::string get_photofile_name(int index) const;

  /// Add thumbnails to the work list.
  /// \param first the index into tree_model_filter of the first icon to add
  /// \param last the index into tree_model_filter of the last icon to add
  void refresh_thumbnails(int first, int last);

  /// Convert an index in the tree_model_filter to an index in the tree_model
  /// \param tree_model_filter_index The index in the tree_model_filter
  /// \return The index in the tree_model
  int tree_model_filter_index_to_tree_model_index(int tree_model_filter_index);

  gboolean tree_model_filter_func(GtkTreeModel *model, GtkTreeIter *unfiltered_iter,
      gpointer data);
  void show_menu_item_activate(GtkMenuItem *menuitem, gpointer user_data);
  void show_tag_menu_item_activate(GtkMenuItem *menuitem, gpointer user_data);
  void change_view_filtering();

  // Static member functions
  static void scroll_view_value_changed_cb(GtkAdjustment *adjustment, gpointer user_data);
  static gboolean idle_cb(gpointer data);
  static void pixbuf_destroy_cb(guchar *pixels, gpointer data);
  static void tab_label_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void clear_button_clicked_cb(GtkWidget *widget, gpointer data);
  static void set_button_clicked_cb(GtkWidget *widget, gpointer data);

  //! A hack to force the GtkIconView to re-layout. It sets it to 1 column then immediately
  //! sets it back to it's correct number of columns.
  static void icon_view_size_allocate_cb(GtkWidget *widget, GdkRectangle *allocation,
      gpointer user_data);
  static gboolean icon_view_key_press_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  //! static callback for a keyboard key press on the GtkIconView icon_view.
  //! \param widget icon_view
  //! \param GdkEvent the event associated with the key press
  //! \param user_data NULL
  gboolean static icon_view_button_press_cb(GtkWidget *widget,
      GdkEvent *event, gpointer user_data);
  static gboolean icon_view_popup_menu_cb(GtkWidget *widget, gpointer user_data);
  static void delete_string(void *vs);
  static void icon_view_popup_extend_selection_activate_cb(GtkMenuItem *menu_item,
      gpointer user_data);
  static void icon_view_popup_open_image_viewer_activate_cb(GtkMenuItem *menu_item,
      gpointer user_data);
  static void icon_view_popup_deactivate_cb(GtkMenuShell *menu_item, gpointer user_data);
  static void icon_view_popup_tag_toggled_cb(GtkMenuItem *menu_item, gpointer user_data);
  static void find_pointer_coords(GtkWidget *widget, gint *x, gint *y);
  static gboolean icon_view_enter_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) ;
  static gboolean icon_view_leave_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);

  static gboolean tree_model_filter_func_cb(GtkTreeModel *model,
      GtkTreeIter *unfiltered_iter, gpointer data);
  static void show_menu_item_activate_cb(GtkMenuItem *menuitem, gpointer user_data);
  static void show_tag_menu_item_activate_cb(GtkMenuItem *menuitem, gpointer user_data);
};
#endif  // MULTIPHOTOPAGE_H__

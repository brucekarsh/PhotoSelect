#ifndef MULTIPHOTOPAGE_H__
#define MULTIPHOTOPAGE_H__

#include "PhotoSelectPage.h"
#include "WidgetRegistry.h"
#include <list>
#include <map>
#include <stdio.h>
#include "ConversionEngine.h"
#include <boost/foreach.hpp>
#include <gtk/gtk.h>
//#include <cairo-xlib.h>
#include <boost/lexical_cast.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

#include "Utils.h"

class PhotoFileCache;
class Preferences;
  
namespace sql {
  class Connection;
}

class MultiPhotoPage : public PhotoSelectPage {
  public:

    //! Holds the state of a single photo on a MultiPhotoPage
    class PhotoState {
      public:
        bool is_selected;
        int pos;
        int  row;
        int col;
        unsigned char *pixels;
        int surface_width;
        int surface_height;
        int rotation;
        PhotoState(bool is_selected = false, int pos = 0, int row = 0, int col = 0) :
            is_selected(is_selected), pos(pos), row(row), col(col), pixels(NULL),
            surface_width(0), surface_height(0) {};
        ~PhotoState() { if (NULL != pixels) {free(pixels);} };
    };

    static const int NUM_COLS = 3;
    static const int DRAWING_AREA_WIDTH = 100;
    static const int DRAWING_AREA_HEIGHT = 100;
    static const int DRAWING_AREA_MARGIN = 4;

    Preferences *thePreferences;
    ConversionEngine conversionEngine;
    std::list<std::string> photoFilenameList;
    std::string project_name;
    sql::Connection *connection;
    PhotoFileCache *photoFileCache;

    GtkWidget *page_hbox;
    GtkWidget *page_vbox;
    GtkWidget *page_left_vbox;
    GtkWidget *page_right_vbox;
    GtkWidget *tab_label_hbox;
    GtkWidget *tab_label_label;
    GtkWidget *tab_label_button;
    GtkWidget *tag_view_box;
    GtkWidget *exif_view_box;
    std::string tags_position;
    std::string exifs_position;
    std::map<std::string, Utils::photo_tag_s> photo_tags;
    std::map<std::string, Utils::project_tag_s> project_tags;
    std::map<GtkWidget *, PhotoState> event_box_map;

  MultiPhotoPage(sql::Connection *connection_, PhotoFileCache *photoFileCache_) :
      conversionEngine(photoFileCache_), 
      thePreferences((Preferences*)0),
      connection(connection_), photoFileCache(photoFileCache_),
      tag_view_box(0),
      exif_view_box(0), tags_position("left"), exifs_position("none") {
  }

  const std::string &get_project_name() {
    return project_name;
  }

  virtual void back() {}
  virtual void next() {}
  virtual void rotate() {}

  PhotoSelectPage *clone() {
    MultiPhotoPage *cloned_photo_select_page = new MultiPhotoPage(connection, photoFileCache);
    cloned_photo_select_page->setup(photoFilenameList, project_name, thePreferences);
    cloned_photo_select_page->set_tags_position(tags_position);
    cloned_photo_select_page->set_exifs_position(exifs_position);
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
    rebuild_tag_view();
  }

  void
  set_exifs_position(const std::string position) {
    exifs_position = position;
    rebuild_exif_view();
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

    // make left and right vboxes to hold meta-information views for things like exif and tags
    page_left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    page_right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(page_left_vbox);
    gtk_widget_show(page_right_vbox);

    // make a vbox to hold the page (page_vbox)
    page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(page_vbox);

    // add the page_left_vbox to the page_hbox
    gtk_box_pack_start(GTK_BOX(page_hbox), page_left_vbox, FALSE, FALSE, 0);
    // add the page_vbox to the page_hbox
    gtk_box_pack_start(GTK_BOX(page_hbox), page_vbox, TRUE, TRUE, 0);
    // add the page_right_vbox to the page_hbox
    gtk_box_pack_start(GTK_BOX(page_hbox), page_right_vbox, FALSE, FALSE, 0);

    // Add the ScrolledWindow
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_SHADOW_ETCHED_OUT);
    gtk_widget_show(GTK_WIDGET(scrolled_window));
    gtk_box_pack_start(GTK_BOX(page_vbox), scrolled_window, TRUE, TRUE, 0);


    // Add the grid
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), true);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), true);
    gtk_widget_show(grid);

    int num_photo_files = photoFilenameList.size();

    for (int i = 0; i < num_photo_files; i++) {
      int row = index_to_row(i);
      int col = index_to_col(i);
      GtkWidget *event_box = gtk_event_box_new();
      gtk_widget_show(event_box);
      g_signal_connect(event_box, "button-press-event", G_CALLBACK(event_box_clicked_cb), NULL);
      GtkWidget *drawing_area = gtk_drawing_area_new();
      g_signal_connect(drawing_area, "draw", G_CALLBACK(drawing_area_draw_cb), NULL);
      g_signal_connect(drawing_area, "realize", G_CALLBACK(drawing_area_realize_cb), NULL);
      gtk_widget_set_margin_left(drawing_area, DRAWING_AREA_MARGIN);
      gtk_widget_set_margin_right(drawing_area, DRAWING_AREA_MARGIN);
      gtk_widget_set_margin_top(drawing_area, DRAWING_AREA_MARGIN);
      gtk_widget_set_margin_bottom(drawing_area, DRAWING_AREA_MARGIN);
      gtk_widget_set_size_request(drawing_area, DRAWING_AREA_WIDTH, DRAWING_AREA_HEIGHT);
      gtk_widget_show(drawing_area);
      gtk_container_add(GTK_CONTAINER(event_box), drawing_area);
      gtk_grid_attach(GTK_GRID(grid), event_box, col, row, 1, 1);
      PhotoState photo_state(false, i, row, col);
      event_box_map[event_box] = photo_state;
    }
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), grid);
    gtk_widget_show(scrolled_window);

    rebuild_tag_view();
    rebuild_exif_view();
  }

  int index_to_row(int index) {
    return index/NUM_COLS;
  }

  int index_to_col(int index) {
    return index%NUM_COLS;
  }

  // Adds a tag view to the MultiPhotoPage. The tag view (tag_view_box) is put into
  // either page_hbox or page_vbox, depending on the tags position (from the view/tags
  // menubar menu.
  // Additionally, it sets up a map (photo_tags) of the tags for the current photo and a
  // list (project_tags) of tags for the current project.
  void rebuild_tag_view() {
    GtkWidget *tag_view_scrolled_window = NULL;
    GtkWidget *tag_view_tags_box = NULL;

    // Destroy any existing tag_view_box
    if (NULL != tag_view_box) {
      gtk_widget_destroy(tag_view_box);
      tag_view_box = NULL;
    }

    // Get all the tags for this project
    project_tags = Utils::get_project_tags(connection, project_name);

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
    //tag_view_tags_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    //gtk_widget_show(tag_view_tags_box);

    // Make a grid to hold the tag
    GtkWidget* tag_view_tags_grid = gtk_grid_new();
    gtk_widget_show(tag_view_tags_grid);

    // Put check buttons in tag_view_tags_box, one for each tag in the project
    int row_num = 0;
    typedef std::pair<std::string, Utils::project_tag_s> map_entry_t;
    BOOST_FOREACH(map_entry_t map_entry, project_tags) {
      std::string name = map_entry.first;
      GtkWidget *label = gtk_label_new(name.c_str());
      gtk_widget_show(label);
      gtk_grid_attach(GTK_GRID(tag_view_tags_grid), label, 0, row_num, 1, 1);
      GtkWidget *set_button = gtk_button_new_with_label("set");
      gtk_widget_show(set_button);
      gtk_grid_attach(GTK_GRID(tag_view_tags_grid), set_button, 1, row_num, 1, 1);
      GtkWidget *clear_button = gtk_button_new_with_label("clear");
      gtk_widget_show(clear_button);
      gtk_grid_attach(GTK_GRID(tag_view_tags_grid), clear_button, 2, row_num, 1, 1);
      row_num++;
    }

    // Put the tag_view_scrolled_window into the tag_view_box.
    gtk_box_pack_start(GTK_BOX(tag_view_box), tag_view_scrolled_window, TRUE, TRUE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tag_view_scrolled_window),
        tag_view_tags_grid);

    // Put the tag_view_box into the page_hbox.
    if (tags_position == "left") {
      gtk_box_pack_start(GTK_BOX(page_left_vbox), tag_view_box, TRUE, TRUE, 0);
    } else if (tags_position == "right") {
      gtk_box_pack_start(GTK_BOX(page_right_vbox), tag_view_box, TRUE, TRUE, 0);
    } else if (tags_position == "top") {
      gtk_box_pack_start(GTK_BOX(page_vbox), tag_view_box, TRUE, TRUE, 0);
      gtk_box_reorder_child(GTK_BOX(page_vbox), tag_view_box, 0);
    } else if (tags_position == "bottom") {
      gtk_box_pack_start(GTK_BOX(page_vbox), tag_view_box, FALSE, FALSE, 0);
    }
  }

  void rebuild_exif_view() {
    GtkWidget *exif_view_scrolled_window = NULL;
    GtkWidget *exif_view_exifs_grid = NULL;

    // Destroy any existing exif_view_box
    if (NULL != exif_view_box) {
      gtk_widget_destroy(exif_view_box);
      exif_view_box = NULL;
    }

    // Get all the exifs for this photo
    std::map<std::string, std::string> exifs = get_exifs();

    // Don't do anything if the exif view is turned off
    if (exifs_position == "none") {
      return;
    }
    
    // Make a box (exif_view_box) for the exif_view
    exif_view_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(exif_view_box);

    // Put a label (exif_view_label) into the exif_view_box
    GtkWidget *exif_view_label = gtk_label_new("Exif Data");
    gtk_widget_show(exif_view_label);
    gtk_box_pack_start(GTK_BOX(exif_view_box), exif_view_label, FALSE, FALSE, 0);
    

    // Make a scrolled window (exif_view_scrolled_window) to scroll the exif list
    exif_view_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(exif_view_scrolled_window),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(exif_view_scrolled_window);

    // Make a grid (exif_view_exifs_grid) to go into the scrolled window
    exif_view_exifs_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(exif_view_exifs_grid), 5);
    gtk_widget_show(exif_view_exifs_grid);

    // Put labels in exif_view_exifs_grid, one pair for each exif
    std::list<std::string> checked_exif_selections =
        thePreferences->get_checked_exif_selections();
    std::list<std::string> text_exif_selections = thePreferences->get_text_exif_selections();
    typedef std::pair<std::string, std::string> map_entry_t;
    int row_num = 0;
    BOOST_FOREACH(map_entry_t map_entry, exifs) {
      std::string exif_name = map_entry.first;
      if ( contains(checked_exif_selections, exif_name) ||
          contains(text_exif_selections, exif_name)) {
        // add this exif entry to the grid
        std::string exif_value = map_entry.second;
        std::string shortened_exif_name = shorten_exif_name(exif_name);
        GtkWidget *name_label = gtk_label_new(shortened_exif_name.c_str());
        gtk_widget_set_tooltip_text(name_label, exif_name.c_str());
        gtk_misc_set_alignment(GTK_MISC(name_label), 0.0, 0.5);
        GtkWidget *value_label = gtk_label_new(exif_value.c_str());
        gtk_widget_set_tooltip_text(value_label, exif_name.c_str());
        gtk_misc_set_alignment(GTK_MISC(value_label), 0.0, 0.5);
        gtk_widget_show(name_label);
        gtk_widget_show(value_label);
        gtk_grid_attach(GTK_GRID(exif_view_exifs_grid), name_label, 0, row_num, 1, 1);
        gtk_grid_attach(GTK_GRID(exif_view_exifs_grid), value_label,  1, row_num, 1, 1);
        row_num += 1;
      }
    }

    // Put the exif_view_scrolled_window into the exif_view_box.
    gtk_box_pack_start(GTK_BOX(exif_view_box), exif_view_scrolled_window, TRUE, TRUE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(exif_view_scrolled_window),
        exif_view_exifs_grid);

    // Put the exif_view_box into the page_hbox.
    if (exifs_position == "left") {
      gtk_box_pack_start(GTK_BOX(page_left_vbox), exif_view_box, TRUE, TRUE, 0);
    } else if (exifs_position == "right") {
      gtk_box_pack_start(GTK_BOX(page_right_vbox), exif_view_box, TRUE, TRUE, 0);
    } else if (exifs_position == "top") {
      gtk_box_pack_start(GTK_BOX(page_vbox), exif_view_box, FALSE, FALSE, 0);
      gtk_box_reorder_child(GTK_BOX(page_vbox), exif_view_box, 0);
    } else if (exifs_position == "bottom") {
      gtk_box_pack_start(GTK_BOX(page_vbox), exif_view_box, FALSE, FALSE, 0);
    }
  }

  std::string shorten_exif_name(const std::string &exif_name) {
    size_t pos = exif_name.find_last_of('.');
    if (std::string::npos == pos) {
      return exif_name;
    }
    return exif_name.substr(pos+1);
  }

  bool contains(std::list<std::string> search_list, std::string search_value) {
    return (std::find(search_list.begin(), search_list.end(), search_value) != search_list.end());
  }
  
  //! Get the exifs from the database and return them as a map from
  //! the exif name to the exif value
  std::map<std::string, std::string> get_exifs() {
    std::map<std::string, std::string> exifs;

    std::string file_name = conversionEngine.getPhotoFilePath();
    std::string exif_string = Utils::get_from_exifblob_by_filePath(connection, file_name);

    std::auto_ptr<xercesc::XercesDOMParser> parser (new xercesc::XercesDOMParser());
    parser->setValidationScheme(xercesc::XercesDOMParser::Val_Never);

    bool ret = parse_json_string(parser.get(), exif_string);
    if (!ret) {
      std::cout << "parse failed" << std::endl;
    } else {
      xercesc::DOMDocument *domDocument = parser->getDocument();
      xercesc::DOMNode * documentElement = domDocument->getDocumentElement();
      
      xercesc::DOMNode *child;
      for (child = documentElement->getFirstChild(); child != NULL;
          child = child->getNextSibling()) {
        char *node_name = xercesc::XMLString::transcode(child->getNodeName());
        xercesc::DOMNode::NodeType node_type = child->getNodeType();
        if (node_type == xercesc::DOMNode::ELEMENT_NODE && !strcmp("t", node_name)) {
          xercesc::DOMNamedNodeMap *attributes = child->getAttributes();
          char *exif_name_value = get_value_by_name(attributes, "name");
          char *exif_value_value = get_value_by_name(attributes, "value");
          if (exif_name_value && exif_value_value) {
            exifs[exif_name_value] = exif_value_value;
          }
        }
      }
    }

    return exifs;
  }

  //!
  //! Returns a value for an attribute value from a json node map given an attribute name
  char *
  get_value_by_name(xercesc::DOMNamedNodeMap *attributes, std::string exif_name) {
    DOMNode *attribute_node = attributes->getNamedItem(X(exif_name.c_str()));
    char *exif_value = NULL;
    if (attribute_node) {
      const XMLCh *value_xmlch = attribute_node->getNodeValue();
      if (value_xmlch) {
        exif_value = xercesc::XMLString::transcode(value_xmlch);
      }
    }
    return exif_value;
  }

  // Used only for debugging
  void print_node(xercesc::DOMNode *node) {
    char *name = xercesc::XMLString::transcode(node->getNodeName());
    char *value = (char *)"NULL";
    const XMLCh *value_xmlch = node->getNodeValue();
    if (value_xmlch) {
      value = xercesc::XMLString::transcode(value_xmlch);
    }
    xercesc::DOMNamedNodeMap *attributes = node->getAttributes();
    XMLSize_t num_attributes = 0;
    if (attributes) {
      num_attributes = attributes->getLength();
    }
    std::cout << node_type_string(node->getNodeType())
        << " " << name
        << " " << value
        << " " << num_attributes
        << std::endl;
    xercesc::XMLString::release(&name);
    for (int i = 0; i < num_attributes; i++) {
      std::cout << "  ";
      print_node(attributes->item(i));
    }
  }

  // Used only for debugging
  std::string node_type_string(xercesc::DOMNode::NodeType nt) {
    static std::string type_strings[] = {
        "",
        "ELEMENT_NODE",
        "ATTRIBUTE_NODE",
        "TEXT_NODE",
        "CDATA_SECTION_NODE",
        "ENTITY_REFERENCE_NODE",
        "ENTITY_NODE",
        "PROCESSING_INSTRUCTION_NODE",
        "COMMENT_NODE",
        "DOCUMENT_NODE",
        "DOCUMENT_TYPE_NODE",
        "DOCUMENT_FRAGMENT_NODE",
        "NOTATION_NODE"
    };

    if (nt < 1 || nt > 12) {
      return "INVALID_NODE";
    }
    return type_strings[nt];
  }

  //!
  //! Parse a json string into a DOM using Xerces
  bool parse_json_string(xercesc::XercesDOMParser* parser, std::string json_string) {
    static const char * memBufId = "someId";
    std::auto_ptr<xercesc::MemBufInputSource> memBufIS (new xercesc::MemBufInputSource(
        (const XMLByte*)json_string.c_str(), json_string.length(), memBufId, false));
    try {
        parser->parse(*memBufIS);
    } catch (const xercesc::XMLException& toCatch) {
        char* message = xercesc::XMLString::transcode(toCatch.getMessage());
        std::cout << "Exception message is: \n"
             << message << "\n";
        xercesc::XMLString::release(&message);
        return false;
    } catch (const xercesc::DOMException& toCatch) {
        char* message = xercesc::XMLString::transcode(toCatch.msg);
        std::cout << "Exception message is: \n"
             << message << "\n";
        xercesc::XMLString::release(&message);
        return false;
    } catch (...) {
        std::cout << "Unexpected Exception \n" ;
        return false;
    }
    return true;
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
    WidgetRegistry<PhotoSelectPage>::set_widget(page_hbox, this);
  } 

  static void drawing_area_realize_cb(GtkWidget *widget, gpointer user_data) {
  }

  static void drawing_area_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    MultiPhotoPage *photoSelectPage =
        (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_draw(widget, cr, user_data);
    }
  }

  void drawing_area_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkDrawingArea *drawing_area = GTK_DRAWING_AREA(widget);
    GtkEventBox *event_box = GTK_EVENT_BOX(gtk_widget_get_parent(GTK_WIDGET(drawing_area)));
    PhotoState &photo_state = event_box_map[GTK_WIDGET(event_box)];
    gint surface_height = gtk_widget_get_allocated_height(GTK_WIDGET(drawing_area));
    gint surface_width = gtk_widget_get_allocated_width(GTK_WIDGET(drawing_area));
    if (photo_state.surface_width == surface_width &&
        photo_state.surface_height == surface_height &&
        NULL != photo_state.pixels) {
      // Do nothing, pixels are already computed
    } else {
      if (NULL != photo_state.pixels) {
        free(photo_state.pixels);
      }
      photo_state.surface_width = surface_width;
      photo_state.surface_height = surface_height;
      get_photo_thumbnail(photo_state);
    }
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, surface_width);
    cairo_surface_t *source_surface = cairo_image_surface_create_for_data(
        photo_state.pixels,
        CAIRO_FORMAT_RGB24, surface_width, surface_height, stride);
    cairo_set_source_surface(cr, source_surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(source_surface);
  }

  void get_photo_thumbnail(PhotoState &photo_state) {
      struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
    conversionEngine.go_to(photo_state.pos);
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
    std::string file_name = conversionEngine.getPhotoFilePath();
    int rotation = Utils::get_rotation(connection, conversionEngine.getPhotoFilePath());
    ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(
        photo_state.surface_width, photo_state.surface_height, rotation); 
    struct timespec t2;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t2);
    double M;
    int width = convertedPhotoFile->width;
    int height = convertedPhotoFile->height;
    calculate_scaling(M, width, height, photo_state.surface_width, photo_state.surface_height);
    photo_state.pixels = convertedPhotoFile->scale_and_pan_and_rotate(
        photo_state.surface_width, photo_state.surface_height, M, 0.0, 0.0, rotation);
    struct timespec t3;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t3);
    std::cout << "Time: goto=" << tdiff(t1,t0) << " getConvertedPhotoFile=" <<
        tdiff(t2,t1) << " scale_and_pan_and_rotate=" <<
        tdiff(t3,t2) << " total=" << tdiff(t3,t0) << std::endl;
  }

  int tdiff(const struct timespec &endtime, const struct timespec &starttime) {
    int tdelta_nsec = endtime.tv_nsec-starttime.tv_nsec;
    int tdelta_sec = endtime.tv_sec-starttime.tv_sec;
    int tdelta = (tdelta_sec * 1000) + (tdelta_nsec / 1000000);
    return tdelta;
  }

  void calculate_scaling(double &M, int image_width, int image_height,
      int surface_width, int surface_height) {
    M = MIN((double) surface_width / image_width, (double) surface_height / image_height);
    if (0.0 == M) {
      M = 1.0;
    }
  }

  static gboolean
  event_box_clicked_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    MultiPhotoPage *photoSelectPage =
        (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->event_box_clicked(widget);
    }
    return TRUE;
  }

  void
  event_box_clicked(GtkWidget *widget) {
    PhotoState &photo_state = event_box_map[widget];
    GtkEventBox *event_box = GTK_EVENT_BOX(widget);
    if (photo_state.is_selected) {
      gtk_widget_override_background_color(widget, GTK_STATE_FLAG_NORMAL, NULL);
      photo_state.is_selected = false;
    } else {
      GdkRGBA rgba;
      gdk_rgba_parse(&rgba, "red");
      gtk_widget_override_background_color(widget, GTK_STATE_FLAG_NORMAL, &rgba);
      photo_state.is_selected = true;
    }
  }
  

  static void tab_label_button_clicked_cb(GtkWidget *widget, gpointer data) {
    MultiPhotoPage *photoSelectPage = (MultiPhotoPage *)data;
    if (0 != photoSelectPage) {
      photoSelectPage->quit();
    }
  }

  void quit();
};

#include "BaseWindow.h"

  inline void MultiPhotoPage::quit() {
    BaseWindow *baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(page_hbox));
    if (NULL != baseWindow) {
      baseWindow->remove_page(page_hbox);
    }
  }
#endif  // MULTIPHOTOPAGE_H__

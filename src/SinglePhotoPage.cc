#include "SinglePhotoPage.h"

#include "BaseWindow.h"
#include "ConvertedPhotoFile.h"
#include "MultiPhotoPage.h"
#include "Preferences.h"
#include "Utils.h"
#include "WidgetRegistry.h"
#include "XStr.h"
#include <list>
#include <map>
#include <stdio.h>
#include <boost/foreach.hpp>
#include <gtk/gtk.h>
#include <boost/lexical_cast.hpp>

using namespace std;

 // ZOOMRATION is 2^(1/4)
#define ZOOMRATIO 1.18920711500272106671

SinglePhotoPage::SinglePhotoPage(PhotoFileCache *photoFileCache_) :
    thePreferences((Preferences*)0),
    rotation(0),
    conversionEngine(photoFileCache_), 
    photoFileCache(photoFileCache_),
    drawing_area(0),
    exif_view_box(0),
    tag_view_box(0),
    Dx(0),
    Dy(0),
    M(1.0),
    drag_is_active(false),
    calculated_initial_scaling(false),
    tags_position("right"),
    exifs_position("right")
  {}

SinglePhotoPage::~SinglePhotoPage() {
  if (page_hbox) {
    // It's important to forget ourself from the WidgetRegistry. If not, we will
    // get odd crashes when our widget's address is reused.
    WidgetRegistry<PhotoSelectPage>::forget_widget(page_hbox);
  }
}

void SinglePhotoPage::load_extra_menu_items() {
}

const string &SinglePhotoPage::get_project_name() {
  return project_name;
}

void SinglePhotoPage::edit_unselect_all_activate() {
}

PhotoSelectPage *SinglePhotoPage::clone() {
  SinglePhotoPage *cloned_photo_select_page = new SinglePhotoPage(photoFileCache);
  cloned_photo_select_page->setup(photoFilenameVector, adjusted_date_time_vector,
      project_name, thePreferences);
  cloned_photo_select_page->set_tags_position(tags_position);
  cloned_photo_select_page->set_exifs_position(exifs_position);
  return cloned_photo_select_page;
}

GtkWidget *
SinglePhotoPage::get_notebook_page() {
  return page_hbox;
}

GtkWidget *
SinglePhotoPage::get_tab_label() {
  return tab_label_hbox;
}

void
SinglePhotoPage::set_tags_position(const string position) {
  tags_position = position;
  rebuild_tag_view();
}

void
SinglePhotoPage::set_exifs_position(const string position) {
  exifs_position = position;
  rebuild_exif_view();
}

void
SinglePhotoPage::build_page() {
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
  string data = ".button {\n"
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

  // add a drawing area (drawing_area) to page_vbox and setup its signals
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
  
  g_signal_connect(drawing_area, "key-press-event",
      G_CALLBACK(drawing_area_key_press_cb), NULL);
  g_signal_connect(drawing_area, "enter-notify-event",
      G_CALLBACK(drawing_area_enter_cb), NULL);
  g_signal_connect(drawing_area, "leave-notify-event",
      G_CALLBACK(drawing_area_leave_cb), NULL);
  gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
      | GDK_SCROLL_MASK | GDK_BUTTON_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
      | GDK_KEY_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

  // make an hbox (button_hbox) to hold the buttons, etc and add it to page_vbox
  button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(button_hbox);
  gtk_box_pack_start(GTK_BOX(page_vbox), button_hbox, FALSE, FALSE, 0);

  // add Next, Back, Rotate and Gimp buttons to button_hbox
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

  rebuild_tag_view();
  rebuild_exif_view();
  g_signal_connect(position_entry, "activate", G_CALLBACK(position_entry_activate_cb), 0);
}

gboolean SinglePhotoPage::drawing_area_key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  bool ret = FALSE;
  guint keyval = ((GdkEventKey *)event)->keyval;
  switch (keyval) {
    case 'n':
      next();
      ret = true;
      break;
    case 'b':
      back();
      ret = true;
      break;
    case 'r':
      rotate();
      ret = true;
      break;
    default:
      break;
  }
  return ret;
}

//! grab the focus when the GtkDrawingArea is entered. This lets it get keyboard events.
//! The grab is removed in drawing_area_leave()
gboolean SinglePhotoPage::drawing_area_enter(GtkWidget *widget, GdkEvent *event,
    gpointer user_data) {
  gtk_grab_add(widget);
  return true;
}

//! un-grab the focus when the GtkDrawingArea is left. This lets it get keyboard events.
//! Focus is  grabbed in drawing_area_enter()
gboolean SinglePhotoPage::drawing_area_leave(GtkWidget *widget, GdkEvent *event,
    gpointer user_data) {
  gtk_grab_remove(widget);
  return true;
}

// Adds a tag view to the SinglePhotoPage. The tag view (tag_view_box) is put into
// either page_hbox or page_vbox, depending on the tags position (from the view/tags
// menubar menu.
// Additionally, it sets up a set (photo_tags) of the tags for the current photo and a
// set (project_tags) of tags for the current project.
void SinglePhotoPage::rebuild_tag_view() {
  GtkWidget *tag_view_scrolled_window = NULL;
  GtkWidget *tag_view_tags_box = NULL;

  // Destroy any existing tag_view_box
  if (NULL != tag_view_box) {
    gtk_widget_destroy(tag_view_box);
    tag_view_box = NULL;
  }

  // Get all the tags for this photo
  string file_name = conversionEngine.getPhotoFilePath();
  db.get_photo_tags_transaction(project_name, file_name, photo_tags);

  // Get all the tags for this project
  db.get_project_tags_transaction(project_name, project_tags);

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
  BOOST_FOREACH(string tag_name, project_tags) {
    // Make a button, pack it, show it and connect it.
    GtkWidget *button = gtk_check_button_new_with_label(tag_name.c_str());
    gtk_box_pack_start(GTK_BOX(tag_view_tags_box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    // If the tag is set for this photo, activate its check button.
    if (1 == photo_tags.count(tag_name)) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
    }

    g_signal_connect(button, "toggled", G_CALLBACK(tag_button_clicked_cb), NULL);
  }

  // Put the tag_view_scrolled_window into the tag_view_box.
  gtk_box_pack_start(GTK_BOX(tag_view_box), tag_view_scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tag_view_scrolled_window),
      tag_view_tags_box);

  // Put the tag_view_box into the page_hbox.
  if (tags_position == "left") {
    gtk_box_pack_start(GTK_BOX(page_left_vbox), tag_view_box, TRUE, TRUE, 0);
  } else if (tags_position == "right") {
    gtk_box_pack_start(GTK_BOX(page_right_vbox), tag_view_box, TRUE, TRUE, 0);
  }
}

void SinglePhotoPage::rebuild_exif_view() {
  GtkWidget *exif_view_scrolled_window = NULL;
  GtkWidget *exif_view_exifs_grid = NULL;

  // Destroy any existing exif_view_box
  if (NULL != exif_view_box) {
    gtk_widget_destroy(exif_view_box);
    exif_view_box = NULL;
  }

  // Get all the exifs for this photo
  map<string, string> exifs = get_exifs();

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
  list<string> checked_exif_selections =
      thePreferences->get_checked_exif_selections();
  list<string> text_exif_selections = thePreferences->get_text_exif_selections();
  typedef pair<string, string> map_entry_t;
  int row_num = 0;
  BOOST_FOREACH(map_entry_t map_entry, exifs) {
    string exif_name = map_entry.first;
    if ( contains(checked_exif_selections, exif_name) ||
        contains(text_exif_selections, exif_name)) {
      // add this exif entry to the grid
      string exif_value = map_entry.second;
      string shortened_exif_name = shorten_exif_name(exif_name);
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
  }
}

string SinglePhotoPage::shorten_exif_name(const string &exif_name) {
  size_t pos = exif_name.find_last_of('.');
  if (string::npos == pos) {
    return exif_name;
  }
  return exif_name.substr(pos+1);
}

bool SinglePhotoPage::contains(list<string> search_list, string search_value) {
  return (find(search_list.begin(), search_list.end(), search_value) != search_list.end());
}

map<string, string> SinglePhotoPage::get_exifs() {
  map<string, string> exifs;

  string file_name = conversionEngine.getPhotoFilePath();
  string exif_string;
  db.get_from_exifblob_by_filePath_transaction(file_name, exif_string);

  unique_ptr<xercesc::XercesDOMParser> parser (new xercesc::XercesDOMParser());
  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Never);

  bool ret = parse_json_string(parser.get(), exif_string);
  if (!ret) {
    cout << "parse failed" << endl;
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
SinglePhotoPage::get_value_by_name(xercesc::DOMNamedNodeMap *attributes, string exif_name) {
  xercesc::DOMNode *attribute_node = attributes->getNamedItem(X(exif_name.c_str()));
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
void SinglePhotoPage::print_node(xercesc::DOMNode *node) {
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
  cout << node_type_string(node->getNodeType())
      << " " << name
      << " " << value
      << " " << num_attributes
      << endl;
  xercesc::XMLString::release(&name);
  for (int i = 0; i < static_cast<int>(num_attributes); i++) {
    cout << "  ";
    print_node(attributes->item(i));
  }
}

// Used only for debugging
string SinglePhotoPage::node_type_string(xercesc::DOMNode::NodeType nt) {
  static string type_strings[] = {
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
bool SinglePhotoPage::parse_json_string(xercesc::XercesDOMParser* parser, string json_string) {
  static const char * memBufId = "someId";
  unique_ptr<xercesc::MemBufInputSource> memBufIS (new xercesc::MemBufInputSource(
      (const XMLByte*)json_string.c_str(), json_string.length(), memBufId, false));
  try {
      parser->parse(*memBufIS);
  } catch (const xercesc::XMLException& toCatch) {
      char* message = xercesc::XMLString::transcode(toCatch.getMessage());
      cout << "Exception message is: \n"
           << message << "\n";
      xercesc::XMLString::release(&message);
      return false;
  } catch (const xercesc::DOMException& toCatch) {
      char* message = xercesc::XMLString::transcode(toCatch.msg);
      cout << "Exception message is: \n"
           << message << "\n";
      xercesc::XMLString::release(&message);
      return false;
  } catch (...) {
      cout << "Unexpected Exception \n" ;
      return false;
  }
  return true;
}


void SinglePhotoPage::setup(vector<string> photoFilenameVector_,
    vector<string> adjusted_date_time_vector_, string project_name_,
    Preferences *thePreferences) {
  this->thePreferences = thePreferences;
  this->project_name = project_name_;
  photoFilenameVector = photoFilenameVector_;
  adjusted_date_time_vector = adjusted_date_time_vector_;

  // Set up a conversion engine.
  conversionEngine.setPhotoFileVector(&photoFilenameVector);

  // Build the page
  build_page();

  // Add it to the registry so we can find this object when we get a callback
  WidgetRegistry<PhotoSelectPage>::set_widget(page_hbox, this);

  // Set up the of_label
  char ofstring[20];
  int numberOfPhotos = photoFilenameVector.size();
  snprintf(ofstring, 20, "of %d      ", numberOfPhotos);
  ofstring[19]=0;
  gtk_label_set_text(GTK_LABEL(of_label), ofstring);

  // Set up the position_entry
  position_entry_activate();
} 

void
SinglePhotoPage::calculate_initial_scaling() {
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

void SinglePhotoPage::zoom(float zoomfactor)
{
  int sx, sy;
  Utils::get_pointer(drawing_area, &sx, &sy);
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
    cout << "ineligible scroll position" << endl;
  }
}


void
SinglePhotoPage::position_entry_activate() {
  string valstr = gtk_entry_get_text(GTK_ENTRY(position_entry));
  int val = atoi(valstr.c_str());
  set_position(val);
}

void
SinglePhotoPage::set_position(int val) {
  if (val < 1) {
    val = 1;
  }
  int siz = photoFilenameVector.size();
  if (val > siz) {
    val = siz;
  }
  conversionEngine.go_to(val-1);   
  db.get_rotation_transaction(conversionEngine.getPhotoFilePath(), rotation);
  calculated_initial_scaling = false;
  set_position_entry();
  gtk_widget_grab_focus(next_button);
  rebuild_tag_view();
  rebuild_exif_view();
  invalidate_image();
}

gboolean
SinglePhotoPage::drawing_area_button_press(GdkEvent *event) {
  int button = event->button.button;
  int event_type = event->type;
  gboolean ret = false;
  if (event_type == GDK_BUTTON_PRESS && button == 1) {
    drag_start_x = event->button.x;
    drag_start_y = event->button.y;
    drag_is_active = true;
    ret = true;
  } else if (event_type == GDK_2BUTTON_PRESS && button == 1) {
    open_multi_photo_page(conversionEngine.get_position());
    ret = true;
  }
  return ret;
}

gboolean
SinglePhotoPage::drawing_area_button_release(GdkEvent *event) {
  int button = event->button.button;
  if (button == 1 && drag_is_active) {
    int drag_end_x = event->button.x;
    int drag_end_y = event->button.y;
    int deltaX = drag_end_x - drag_start_x;
    int deltaY = drag_end_y - drag_start_y;
    Dx += deltaX;
    Dy += deltaY;
    invalidate_image();
    drag_is_active = false;
    return true;
  }
  return false;
}

gboolean
SinglePhotoPage::drawing_area_scroll(GdkEvent *event) {
  if (event->scroll.direction == GDK_SCROLL_UP) {
    zoom(ZOOMRATIO);
  } else if (event->scroll.direction == GDK_SCROLL_DOWN) {
    zoom(1.0/ZOOMRATIO);
  } else if (event->scroll.direction == GDK_SCROLL_SMOOTH) {
    // ignore GDK_SCROLL_SMOOTH
  } else {
    cout << "don't know which way to scroll" << endl;
  }
  invalidate_image();
  return true;
}

void
SinglePhotoPage::drawing_area_motion_notify(GdkEvent * event) {
  if (drag_is_active) {
      int drag_end_x = event->motion.x;
      int drag_end_y = event->motion.y;
      int deltaX = drag_end_x - drag_start_x;
      int deltaY = drag_end_y - drag_start_y;
      Dx += deltaX;
      Dy += deltaY;
      drag_start_x = drag_end_x;
      drag_start_y = drag_end_y;
      invalidate_image();
  }
}

/** Sets to position_entry widget to reflect the positioon of the ConversionEngine.
    The ConversionEngine counts 0..N-1 and the position_entry counts 1..N. */
void
SinglePhotoPage::set_position_entry() {
  int val = conversionEngine.get_position();
  string valstring =  boost::lexical_cast<string>(val + 1);
  gtk_entry_set_text(GTK_ENTRY(position_entry), valstring.c_str());
}

void
SinglePhotoPage::invalidate_image() {
  gint surface_height = gtk_widget_get_allocated_height(drawing_area);
  gint surface_width = gtk_widget_get_allocated_width(drawing_area);
  gtk_widget_queue_draw_area(drawing_area, 0, 0, surface_width, surface_height);
}

void SinglePhotoPage::redraw_image() {
  ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(); 
  string photoFilePath = conversionEngine.getPhotoFilePath();
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

void SinglePhotoPage::next() {
  conversionEngine.next();   
  db.get_rotation_transaction(conversionEngine.getPhotoFilePath(), rotation);
  calculated_initial_scaling = false;
  set_position_entry();
  rebuild_tag_view();
  rebuild_exif_view();
  invalidate_image();
  // make sure that the drawing_area retains the keyboard focus (the rebuilds sometimes steal it)
  gtk_grab_add(drawing_area);

}

void SinglePhotoPage::back() {
  conversionEngine.back();   
  db.get_rotation_transaction(conversionEngine.getPhotoFilePath(), rotation);
  calculated_initial_scaling = false;
  set_position_entry();
  rebuild_tag_view();
  rebuild_exif_view();
  invalidate_image();
  // make sure that the drawing_area retains the keyboard focus (the rebuilds sometimes steal it)
  gtk_grab_add(drawing_area);
}

void SinglePhotoPage::rotate() {
  rotation += 1;
  if (rotation == 4) {
    rotation = 0;
  }
  db.set_rotation_transaction(conversionEngine.getPhotoFilePath(), rotation);
  invalidate_image();
  // make sure that the drawing_area retains the keyboard focus (the rebuilds sometimes steal it)
  gtk_grab_add(drawing_area);
}

void SinglePhotoPage::gimp() {
  // TODO WRITEME or DELETEME
}

void
SinglePhotoPage::tag_button_clicked(GtkToggleButton *togglebutton, gpointer user_data) {
  string tag_name = gtk_button_get_label(GTK_BUTTON(togglebutton));
  bool active = gtk_toggle_button_get_active(togglebutton);
  string file_name = conversionEngine.getPhotoFilePath();
  db.get_project_tags_transaction(project_name, project_tags);
  db.get_photo_tags_transaction(project_name, file_name, photo_tags);

  // Ignore this click if it's for a tag that's not in our project
  if (0 == project_tags.count(tag_name)) {
    return;
  }

  if (active) {
    bool b  = db.add_tag_by_filename_transaction(tag_name, file_name);
    if (!b) {
      // TODO Handle db.add_tag_by_filename_transaction failure
    }
  } else {
    bool b = db.remove_tag_by_filename_transaction(tag_name, file_name);
    if (!b) {
      // TODO Handle db.remove_tag_by_filename_transaction failure
    }
  }
}

void SinglePhotoPage::quit() {
  BaseWindow *baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(drawing_area));
  if (NULL != baseWindow) {
    baseWindow->remove_page(page_hbox);
  }
  delete this;
}

void SinglePhotoPage::add_page_to_base_window(PhotoSelectPage *photo_page) {
  BaseWindow *baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(page_hbox));
  if (NULL != baseWindow) {
    baseWindow->add_page(photo_page->get_tab_label(),
        photo_page->get_notebook_page(), project_name);
  }
};

void SinglePhotoPage::open_multi_photo_page(int index) {
  MultiPhotoPage *multi_photo_page = new MultiPhotoPage(photoFileCache);
  multi_photo_page->setup(photoFilenameVector, adjusted_date_time_vector, project_name, thePreferences);
  multi_photo_page->set_position(index+1); // (set_position is 1-based)
  add_page_to_base_window(multi_photo_page);
}

// Static member functions

/* static */ gboolean SinglePhotoPage::drawing_area_key_press_cb(GtkWidget *widget, GdkEvent *event,
    gpointer user_data) {
  SinglePhotoPage *photoSelectPage =
      (SinglePhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage->drawing_area_key_press(widget, event, user_data);
  }
  return false;
}

/* static */ gboolean
SinglePhotoPage::drawing_area_enter_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  SinglePhotoPage *photoSelectPage =
      (SinglePhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage-> drawing_area_enter(widget, event, user_data);
  }
  return false;
}

/* static */ gboolean
SinglePhotoPage::drawing_area_leave_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  SinglePhotoPage *photoSelectPage =
      (SinglePhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage-> drawing_area_leave(widget, event, user_data);
  }
  return false;
}

/* static */ void SinglePhotoPage::tab_label_button_clicked_cb(GtkWidget *widget, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)data;
  if (0 != photoSelectPage) {
    photoSelectPage->quit();
  }
}

/* static */ void
SinglePhotoPage::next_button_clicked_cb(GtkWidget *widget, gpointer data) {
  SinglePhotoPage *photoSelectPage =
      (SinglePhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage -> next();
  }
}

/* static */ void
SinglePhotoPage::back_button_clicked_cb(GtkWidget *widget, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage -> back();
  }
}

/* static */ void
SinglePhotoPage::rotate_button_clicked_cb(GtkWidget *widget, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage -> rotate();
  }
}

/* static */ void
SinglePhotoPage::gimp_button_clicked_cb(GtkWidget *widget, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage -> gimp();
  }
}

/* static */ void
SinglePhotoPage::drawing_area_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage -> redraw_image();
  }
}

/* static */ void
SinglePhotoPage::position_entry_activate_cb(GtkWidget *widget, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->position_entry_activate();
  }
}

/* static */ gboolean
SinglePhotoPage::drawing_area_button_press_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage->drawing_area_button_press(event);
  }
  return false;
}

/* static */ gboolean
SinglePhotoPage::drawing_area_button_release_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage->drawing_area_button_release(event);
  }
  return false;
}

/* static */ gboolean
SinglePhotoPage::drawing_area_scroll_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage->drawing_area_scroll(event);
  }
  return false;
}

/* static */ void
SinglePhotoPage::drawing_area_motion_notify_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->drawing_area_motion_notify(event);
  }
}

/* static */ void
SinglePhotoPage::tag_button_clicked_cb(GtkToggleButton *togglebutton, gpointer user_data) {
  SinglePhotoPage *photoSelectPage = (SinglePhotoPage *)
      WidgetRegistry<PhotoSelectPage>::get_object(
      GTK_WIDGET(togglebutton));
  if (0 != photoSelectPage) {
    photoSelectPage->tag_button_clicked(togglebutton, user_data);
  }
}

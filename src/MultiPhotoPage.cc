#include "MultiPhotoPage.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "BaseWindow.h"
#include "ConvertedPhotoFile.h"
#include "Preferences.h"
#include "SinglePhotoPage.h"
#include "Utils.h"
#include "WidgetRegistry.h"
#include "XStr.h"


using namespace std;

MultiPhotoPage::MultiPhotoPage(PhotoFileCache *photoFileCache_) :
    conversionEngine(photoFileCache_), 
    thePreferences((Preferences*)0),
    photoFileCache(photoFileCache_),
    tag_view_box(0), current_index(0),
    exif_view_box(0), tags_position("right"), exifs_position("right"),
    view_filter_show_all(true), view_filter_show(false), view_filter_dont_show(false) {
  ticket_number = ticket_registry.new_ticket();
}

MultiPhotoPage::~MultiPhotoPage() {
  // Get rid of all our work on the WorkList. (There still may be some work in progress)
  work_list.delete_work_by_ticket_number(ticket_number);
  // Unreference our ticket_number and wait for everyone else to do so as well
  // That ensures that we cannot get callbacks from the Workers. The work in progress
  // will be balked by the Workers because they can't get a reference to the ticket.
  ticket_registry.unreference_ticket(ticket_number);
  ticket_registry.wait_for_ticket(ticket_number);
  {
    boost::lock_guard<boost::mutex> member_lock(class_mutex); 
    if (idle_id) {
      g_source_remove(idle_id);
    }
  }
  if (page_hbox) {
    // It's important to forget ourself from the WidgetRegistry. If not, we will
    // get odd crashes when our widget's address is reused.
    WidgetRegistry<PhotoSelectPage>::forget_widget(page_hbox);
  }
}

const string &MultiPhotoPage::get_project_name() {
  return project_name;
}

void MultiPhotoPage::rotate(int index) {
  // Set the rotation in the db
  PhotoState &photo_state = photo_state_map[index];
  string file_path = photoFilenameVector[photo_state.get_index()];
  int rotation = photo_state.get_rotation();
  rotation += 1;
  if (rotation == 4) {
    rotation = 0;
  }
  Db::set_rotation_transaction(file_path, rotation);
  photo_state.clear_pixbuf();

  // Get a rotated thumbnail and put it tin the photo_state
  get_photo_thumbnail(photo_state, ICON_WIDTH, ICON_HEIGHT);

  // Update the list_store
  GtkTreePath *path =
      gtk_tree_path_new_from_string(boost::lexical_cast<string>(index).c_str());
  GtkTreeIter iter;
  gtk_tree_model_get_iter(GTK_TREE_MODEL(list_store), &iter, path);
  gtk_list_store_set(list_store, &iter, COL_PIXBUF, photo_state.get_pixbuf(), -1);
  gtk_tree_path_free(path);
  announce_row_change(index);
}

void MultiPhotoPage::announce_row_change(int index) {
  if (is_visible_thumbnail(index)) {
    GtkTreePath *path =
        gtk_tree_path_new_from_string(boost::lexical_cast<string>(index).c_str());
    GtkTreePath *filtered_path = gtk_tree_model_filter_convert_child_path_to_path(
        GTK_TREE_MODEL_FILTER(tree_model_filter), path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(GTK_TREE_MODEL(tree_model_filter), &iter, filtered_path);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(tree_model_filter), filtered_path, &iter);
    gtk_tree_path_free(filtered_path);
  }
}

void MultiPhotoPage::edit_unselect_all_activate() {
  int index = 0;
  // Iterate through each photo file in the project
  BOOST_FOREACH(string filename, photoFilenameVector) {
    select_thumbnail_by_index(index, false);
    index++;
  }
}

void MultiPhotoPage::select_thumbnail_by_index(int index, bool new_state) {
  PhotoState &photo_state = photo_state_map[index];
  if (new_state) {
    if (!photo_state.get_is_selected()) {
      // set it in the PhotoState
      photo_state.set_is_selected(true);
      //set it in the icon view
      GtkTreePath *path =
          gtk_tree_path_new_from_string(boost::lexical_cast<string>(index).c_str());
      GtkTreePath *filtered_path = gtk_tree_model_filter_convert_child_path_to_path(
          GTK_TREE_MODEL_FILTER(tree_model_filter), path);
      if (filtered_path) {
        gtk_icon_view_select_path(GTK_ICON_VIEW(icon_view), filtered_path);
        gtk_tree_path_free(filtered_path);
      }
      gtk_tree_path_free(path);
    }
  } else /* !new_state */{
    if (photo_state.get_is_selected()) {
      // unset it in the PhotoState
      photo_state.set_is_selected(false);
      //unset it in the icon view
      GtkTreePath *path =
          gtk_tree_path_new_from_string(boost::lexical_cast<string>(index).c_str());
      GtkTreePath *filtered_path = gtk_tree_model_filter_convert_child_path_to_path(
          GTK_TREE_MODEL_FILTER(tree_model_filter), path);
      if (filtered_path) {
        gtk_icon_view_unselect_path(GTK_ICON_VIEW(icon_view), filtered_path);
        gtk_tree_path_free(filtered_path);
      }
      gtk_tree_path_free(path);
    }
  }
}

PhotoSelectPage *MultiPhotoPage::clone() {
  MultiPhotoPage *cloned_photo_select_page = new MultiPhotoPage(photoFileCache);
  cloned_photo_select_page->setup(photoFilenameVector, adjusted_date_time_vector,
      project_name, thePreferences);
  cloned_photo_select_page->set_tags_position(tags_position);
  cloned_photo_select_page->set_exifs_position(exifs_position);
  return cloned_photo_select_page;
}

GtkWidget *
MultiPhotoPage::get_notebook_page() {
  return page_hbox;
}

GtkWidget *
MultiPhotoPage::get_tab_label() {
  return tab_label_hbox;
}

void
MultiPhotoPage::set_tags_position(const string position) {
  tags_position = position;
  rebuild_tag_view();
}

void
MultiPhotoPage::set_exifs_position(const string position) {
  exifs_position = position;
  rebuild_exif_view();
}

void
MultiPhotoPage::build_page() {
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

  // make an hbox to hold the page (page_hbox)
  page_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_events(page_hbox, GDK_STRUCTURE_MASK | GDK_CONFIGURE);
  gtk_widget_show(page_hbox);

  // make left and right vboxes to hold meta-information views for things like exif and tags
  page_left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  page_right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(page_left_vbox);
  gtk_widget_show(page_right_vbox);

  // make left and right hboxes for exif and tags
  tags_left_hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  tags_right_hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  exif_left_hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  exif_right_hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(tags_left_hbox);
  gtk_widget_show(tags_right_hbox);
  gtk_widget_show(exif_left_hbox);
  gtk_widget_show(exif_right_hbox);

  // make a vbox to hold the page (page_vbox)
  page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(page_vbox);

  // add the page_left_vbox to the page_hbox
  gtk_box_pack_start(GTK_BOX(page_hbox), page_left_vbox, FALSE, FALSE, 0);
  // add the page_vbox to the page_hbox
  gtk_box_pack_start(GTK_BOX(page_hbox), page_vbox, TRUE, TRUE, 0);
  // add the page_right_vbox to the page_hbox
  gtk_box_pack_start(GTK_BOX(page_hbox), page_right_vbox, FALSE, FALSE, 0);

  // add the exif and tags hboxes to the left and right vboxes
  gtk_box_pack_start(GTK_BOX(page_left_vbox), tags_left_hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(page_left_vbox), exif_left_hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(page_right_vbox), tags_right_hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(page_right_vbox), exif_right_hbox, TRUE, TRUE, 0);

  // Add the ScrolledWindow
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
      GTK_SHADOW_ETCHED_OUT);
  gtk_widget_show(GTK_WIDGET(scrolled_window));
  central_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(central_hbox);
  gtk_box_pack_start(GTK_BOX(central_hbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(page_vbox), central_hbox, TRUE, TRUE, 0);

  // Find the GtkScrolledWindow's vadjustment so we can watch for visiblity changes
  // in the GtkIconVew
  GtkAdjustment *vadjustment =
      gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));

  // Add the GtkIconView
  list_store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
  GtkTreeModel *tree_model = GTK_TREE_MODEL(list_store);
  tree_model_filter = gtk_tree_model_filter_new(tree_model, NULL);
  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(tree_model_filter),
      tree_model_filter_func_cb, (gpointer)this, NULL);
  icon_view = gtk_icon_view_new_with_model (tree_model_filter);
  gtk_stock_missing_image = stock_thumbnails->get_loading_thumbnail();

  int num_photo_files = photoFilenameVector.size();

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  long priority = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

  // Iterate over the photo files, make PhotoStates, set up the initial (missing image) icons,
  // and captions
  GtkTreeIter iter;
  for (int i = 0; i < num_photo_files; i++) {
    string adjusted_date_time = adjusted_date_time_vector[i];
    photo_state_map[i] = PhotoState(false, i);
    photo_state_map[i].set_pixbuf(gtk_stock_missing_image, 0);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, COL_PIXBUF, gtk_stock_missing_image, -1);
    string caption((
        "[" +
        boost::lexical_cast<string>(i + 1) +
        "]    " +
        "<b>" +
        adjusted_date_time +
        "</b>"
    ));
    gtk_list_store_set(list_store, &iter, COL_MARKUP, caption.c_str(), -1);
    announce_row_change(i);
  }

  // Iterate over the photo files make WorkItems and send them to the WorkList so the workers
  // can fetch and transform them. We iterate in reverse order because images are rendered
  // mostly last-in first-out. So this causes them render from the top of the page towards
  // the bottom.
  for (int i = num_photo_files - 1; i >= 0; i--) {
    int rotation;
    Db::get_rotation_transaction(photoFilenameVector[i], rotation);
    WorkItem work_item(ticket_number, i,rotation, priority, this);
    work_list.add_work(work_item);
  }
  gtk_widget_add_events(icon_view, GDK_KEY_PRESS_MASK | GDK_ENTER_NOTIFY_MASK
      | GDK_LEAVE_NOTIFY_MASK);

  g_signal_connect(icon_view, "key-press-event", G_CALLBACK(icon_view_key_press_cb), 0);
  g_signal_connect(icon_view, "size_allocate", G_CALLBACK(icon_view_size_allocate_cb), 0);
  g_signal_connect(icon_view, "button-press-event", G_CALLBACK(icon_view_button_press_cb), 0);
  g_signal_connect(icon_view, "popup-menu", G_CALLBACK(icon_view_popup_menu_cb), 0);
  g_signal_connect(icon_view, "enter-notify-event", G_CALLBACK(icon_view_enter_cb), NULL);
  g_signal_connect(icon_view, "leave-notify-event", G_CALLBACK(icon_view_leave_cb), NULL);
  g_signal_connect(vadjustment, "value-changed", G_CALLBACK(scroll_view_value_changed_cb),
      (gpointer) this);

  gtk_icon_view_set_spacing(GTK_ICON_VIEW(icon_view), 0);	// space between thumbnail and caption
  gtk_icon_view_set_item_width(GTK_ICON_VIEW(icon_view), ICON_WIDTH);
  gtk_icon_view_set_row_spacing(GTK_ICON_VIEW(icon_view), 3); // space between caption and next thumbnail
  gtk_icon_view_set_column_spacing(GTK_ICON_VIEW(icon_view), 4); // space between columns
  gtk_icon_view_set_margin(GTK_ICON_VIEW(icon_view), 0); // space around the whole icon view
  gtk_icon_view_set_item_padding(GTK_ICON_VIEW(icon_view), 4);
  gtk_icon_view_set_columns(GTK_ICON_VIEW(icon_view), -1);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_MULTIPLE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), icon_view);
  gtk_widget_show_all(scrolled_window);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon_view), COL_PIXBUF);
  gtk_icon_view_set_markup_column (GTK_ICON_VIEW (icon_view), COL_MARKUP);
  gtk_widget_show(scrolled_window);

  rebuild_tag_view();
  rebuild_exif_view();
}

/* static */ void MultiPhotoPage::scroll_view_value_changed_cb(GtkAdjustment *adjustment, gpointer user_data) {
  MultiPhotoPage *photoSelectPage = (MultiPhotoPage *) user_data;
  if (0 != photoSelectPage) {
    photoSelectPage->scroll_view_value_changed(adjustment, user_data);
  }
}

void MultiPhotoPage::scroll_view_value_changed(GtkAdjustment *adjustment, gpointer user_data) {
  GtkTreePath *start_path;
  GtkTreePath *end_path;
  gboolean b = gtk_icon_view_get_visible_range(GTK_ICON_VIEW(icon_view), &start_path, &end_path);
  // start_path and end_path are paths in tree_model_filter. (not in tree_model)
  if (b) {
    int start_pos = atoi(gtk_tree_path_to_string(start_path));
    int end_pos = atoi(gtk_tree_path_to_string(end_path));
    refresh_thumbnails(start_pos, end_pos);
  } else {
    cout << "not available" << endl;
  }
}

/* static */ gboolean MultiPhotoPage::idle_cb(gpointer data) {
  MultiPhotoPage *multiPhotoPage = (MultiPhotoPage *)data;
  BOOST_ASSERT(multiPhotoPage);
  if (multiPhotoPage) {
    return multiPhotoPage->idle();
  }
  return false;
}

gboolean MultiPhotoPage::idle() {
  int index;
  int rotation;
  bool is_now_empty;
  GdkPixbuf *pixbuf;
  {
    boost::lock_guard<boost::mutex> member_lock(class_mutex); 
    BOOST_ASSERT(!pixbuf_map.empty());

    // Iterate through the pixbuf_map and find the entry with the largest priority
    map<int, PixbufMapEntry>::iterator it;
    map<int, PixbufMapEntry>::iterator largest_priority_it = pixbuf_map.begin();
    long largest_priority = 0;
    int largest_priority_n = 0;
    int n = 0;
    for (it = pixbuf_map.begin(); it != pixbuf_map.end(); ++it) {
      long priority = (it->second).priority;
      if (priority > largest_priority) {
        largest_priority = priority;
        largest_priority_it = it;
        largest_priority_n = n;
      }
      n++;
    }
    it = largest_priority_it;
    index = it->first;
    pixbuf = (it->second).pixbuf;
    rotation = (it->second).rotation;
    pixbuf_map.erase(it);
    if (pixbuf_map.empty()) {
      g_source_remove(idle_id);
      idle_id = 0;
    }
  }
  apply_thumbnail(index, pixbuf, rotation);
  return true;
}

//! Puts a thumbnail in the pixbuf_map. Thumbnails are then rendered via the idle callback.
//! We don't render them asynchronously from the worker thread because that causes too much
//! flashing.
bool MultiPhotoPage::set_thumbnail(int index, GdkPixbuf *pixbuf, int rotation, long priority) {
  boost::lock_guard<boost::mutex> member_lock(class_mutex); 
  // If the queue is too large, balk and the worker will try again later
  int qsize = pixbuf_map.size();
  if (qsize >= 10) {
    return false;
  }
  if (pixbuf_map.empty()) {
    idle_id = g_idle_add(idle_cb, (gpointer)this);
  }
  pixbuf_map[index] = PixbufMapEntry(pixbuf, rotation, priority);
  return true;
}

//! Puts a thumbnail into the photo_state and into the list_store
//! \param index the index of the thumbnail in the (unfiltered) list_store.
//! \param pixbuf the pixbuf to put into the photo_state and list_store
//! \param rotation rotation of the put copy [0= unrotated, 1=90, 2=180, 3=270. counterclockwise]
void MultiPhotoPage::apply_thumbnail(int index, GdkPixbuf *pixbuf, int rotation) {
  PhotoState &photo_state = photo_state_map[index];
  photo_state.set_pixbuf(pixbuf, rotation);
  gdk_threads_enter();
  GtkTreePath *path =
      gtk_tree_path_new_from_string(boost::lexical_cast<string>(index).c_str());
  GtkTreeIter iter; // an iter into the (unfiltered) list_store
  gtk_tree_model_get_iter(GTK_TREE_MODEL(list_store), &iter, path);
  gtk_tree_path_free(path);
  gtk_list_store_set(list_store, &iter, COL_PIXBUF, photo_state.get_pixbuf(), -1);
  announce_row_change(index);
  gdk_threads_leave();
}

/* static */ void MultiPhotoPage::pixbuf_destroy_cb(guchar *pixels, gpointer data) {
  free(pixels);
}

// Adds a tag view to the MultiPhotoPage. The tag view (tag_view_box) is put into
// either page_hbox or page_vbox, depending on the tags position (from the view/tags
// menubar menu.
// Additionally, it sets up a map (photo_tags) of the tags for the current photo and a
// list (project_tags) of tags for the current project.
void MultiPhotoPage::rebuild_tag_view() {
  GtkWidget *tag_view_scrolled_window = NULL;
  GtkWidget *tag_view_tags_box = NULL;

  // Destroy any existing tag_view_box
  if (NULL != tag_view_box) {
    gtk_widget_destroy(tag_view_box);
    tag_view_box = NULL;
  }

  // Get all the tags for this project
  Db::get_project_tags_transaction(project_name, project_tags);

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

  // Make a grid to hold the tag
  GtkWidget* tag_view_tags_grid = gtk_grid_new();
  gtk_widget_show(tag_view_tags_grid);

  // Put check buttons in tag_view_tags_box, one for each tag in the project
  int row_num = 0;
  tag_button_map.clear();
  string file_name = photoFilenameVector[current_index];
  typedef pair<string, Db::project_tag_s> map_entry_t;
  // all_photo_tags_for_project[file_name][tag_name]
  set<string> tag_map = all_photo_tags_for_project[file_name];
  BOOST_FOREACH(map_entry_t map_entry, project_tags) {
    string name = map_entry.first;
    if (tag_map.count(name)) {
      GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);
      gtk_widget_show(image);
      gtk_grid_attach(GTK_GRID(tag_view_tags_grid), image, 0, row_num, 1, 1);
    }
    string display_text(name + " (" +
        boost::lexical_cast<string>(all_tag_counts[name]) + ")");
    GtkWidget *label = gtk_label_new(display_text.c_str());
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    gtk_grid_attach(GTK_GRID(tag_view_tags_grid), label, 1, row_num, 1, 1);
    string set_label = "set (" +
        boost::lexical_cast<string>(set_tag_counts[name]) + ")";
    GtkWidget *set_button = gtk_button_new_with_label(set_label.c_str());
    tag_button_map[set_button] = name;
    g_signal_connect(set_button, "clicked", G_CALLBACK(set_button_clicked_cb), NULL);
    gtk_widget_show(set_button);
    gtk_grid_attach(GTK_GRID(tag_view_tags_grid), set_button, 3, row_num, 1, 1);
    string clear_label = "clear (" +
        boost::lexical_cast<string>(clear_tag_counts[name]) + ")";
    GtkWidget *clear_button = gtk_button_new_with_label(clear_label.c_str());
    tag_button_map[clear_button] = name;
    g_signal_connect(clear_button, "clicked", G_CALLBACK(clear_button_clicked_cb), NULL);
    gtk_widget_show(clear_button);
    gtk_grid_attach(GTK_GRID(tag_view_tags_grid), clear_button, 4, row_num, 1, 1);
    row_num++;
  }
  int num_rows = row_num;
  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_margin_left(separator, 5);
  gtk_widget_set_margin_right(separator, 5);
  gtk_widget_show(separator);
  gtk_grid_attach(GTK_GRID(tag_view_tags_grid), separator, 2, 0, 1, num_rows);

  // Put the tag_view_scrolled_window into the tag_view_box.
  gtk_box_pack_start(GTK_BOX(tag_view_box), tag_view_scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tag_view_scrolled_window),
      tag_view_tags_grid);

  // Put the tag_view_box into the page_hbox.
  if (tags_position == "left") {
    gtk_box_pack_start(GTK_BOX(tags_left_hbox), tag_view_box, TRUE, TRUE, 0);
  } else if (tags_position == "right") {
    gtk_box_pack_start(GTK_BOX(tags_right_hbox), tag_view_box, TRUE, TRUE, 0);
  } 
}

void MultiPhotoPage::count_tags() {
  all_tag_counts.clear();
  set_tag_counts.clear();
  clear_tag_counts.clear();
  // Get the tags for all the files in this project
  Db::get_all_photo_tags_for_project_transaction(project_name, all_photo_tags_for_project);

  // Count the tags
  BOOST_FOREACH(Db::all_photo_tags_map_entry_t map_entry, all_photo_tags_for_project) {
    BOOST_FOREACH(string e, map_entry.second) {
      all_tag_counts[e] += 1;
    }
  }

  int index = 0;
  // Iterate through each photo file in the project
  BOOST_FOREACH(string filename, photoFilenameVector) {
    // get the photo file's PhotoState
    PhotoState &photo_state = photo_state_map[index];
    // we want counts of the number of tags that will be newly set and cleared
    // so we only want to look at selected photos
    if (photo_state.get_is_selected()) {
      // get all of the tags for the photo
      set<string> photo_tags = all_photo_tags_for_project[filename];
      typedef pair<string, Db::project_tag_s> map_entry_t;
      BOOST_FOREACH(map_entry_t map_entry, project_tags) {
        string tag_name = map_entry.first;
        if (0 != photo_tags.count(tag_name)) {
          clear_tag_counts[tag_name]++;
        } else {
          set_tag_counts[tag_name]++;
        }
      }
    }
    index++;
  }
}

void MultiPhotoPage::rebuild_exif_view() {
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
    gtk_box_pack_start(GTK_BOX(exif_left_hbox), exif_view_box, TRUE, TRUE, 0);
  } else if (exifs_position == "right") {
    gtk_box_pack_start(GTK_BOX(exif_right_hbox), exif_view_box, TRUE, TRUE, 0);
  }
}

string MultiPhotoPage::shorten_exif_name(const string &exif_name) {
  size_t pos = exif_name.find_last_of('.');
  if (string::npos == pos) {
    return exif_name;
  }
  return exif_name.substr(pos+1);
}

bool MultiPhotoPage::contains(list<string> search_list, string search_value) {
  return (find(search_list.begin(), search_list.end(), search_value) != search_list.end());
}

//! Get the exifs from the database and return them as a map from
//! the exif name to the exif value
map<string, string> MultiPhotoPage::get_exifs() {
  map<string, string> exifs;

  string file_name = photoFilenameVector[current_index];
  string exif_string;
  Db::get_from_exifblob_by_filePath_transaction(file_name, exif_string);

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
MultiPhotoPage::get_value_by_name(xercesc::DOMNamedNodeMap *attributes, string exif_name) {
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
void MultiPhotoPage::print_node(xercesc::DOMNode *node) {
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
  for (int i = 0; i < num_attributes; i++) {
    cout << "  ";
    print_node(attributes->item(i));
  }
}

// Used only for debugging
string MultiPhotoPage::node_type_string(xercesc::DOMNode::NodeType nt) {
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
bool MultiPhotoPage::parse_json_string(xercesc::XercesDOMParser* parser, string json_string) {
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


void MultiPhotoPage::setup(vector<string> photoFilenameVector_,
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
  count_tags();
  rebuild_tag_view();

  // Add it to the registry so we can find this object when we get a callback
  WidgetRegistry<PhotoSelectPage>::set_widget(page_hbox, this);
} 

void MultiPhotoPage::get_photo_thumbnail(PhotoState &photo_state, int surface_width, int surface_height) {
    struct timespec t0;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
  conversionEngine.go_to(photo_state.get_index());
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
  string file_name = conversionEngine.getPhotoFilePath();
  int rotation;
  Db::get_rotation_transaction(conversionEngine.getPhotoFilePath(), rotation);
  ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(
      surface_width, surface_height, rotation); 
  struct timespec t2;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t2);
  double M;
  int width = convertedPhotoFile->width;
  int height = convertedPhotoFile->height;
  calculate_scaling(M, width, height, surface_width, surface_height);
  unsigned char *pixels = convertedPhotoFile->scale_and_pan_and_rotate(
      surface_width, surface_height, M, 0.0, 0.0, rotation);
  struct timespec t3;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t3);
  cout << "Time: goto=" << tdiff(t1,t0) << " getConvertedPhotoFile=" <<
      tdiff(t2,t1) << " scale_and_pan_and_rotate=" <<
      tdiff(t3,t2) << " total=" << tdiff(t3,t0) << endl;
  
  // Convert pixels to the format favored by GtkIconView
  unsigned char *newpixels = (unsigned char *)malloc(surface_width * surface_height * 3);
  unsigned char *p = newpixels;
  for (int i = 0; i < surface_width * surface_height; i++) {
    *p++ = pixels[4*i+2];
    *p++ = pixels[4*i+1];
    *p++ = pixels[4*i+0];
  }
  free(pixels);
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(newpixels, GDK_COLORSPACE_RGB,
      FALSE, 8, ICON_WIDTH, ICON_HEIGHT, ICON_STRIDE, pixbuf_destroy_cb, NULL);
  photo_state.set_pixbuf(pixbuf, rotation);
}

int MultiPhotoPage::tdiff(const struct timespec &endtime, const struct timespec &starttime) {
  int tdelta_nsec = endtime.tv_nsec-starttime.tv_nsec;
  int tdelta_sec = endtime.tv_sec-starttime.tv_sec;
  int tdelta = (tdelta_sec * 1000) + (tdelta_nsec / 1000000);
  return tdelta;
}

void MultiPhotoPage::calculate_scaling(double &M, int image_width, int image_height,
    int surface_width, int surface_height) const {
  M = MIN((double) surface_width / image_width, (double) surface_height / image_height);
  if (0.0 == M) {
    M = 1.0;
  }
}

/* static */ void MultiPhotoPage::tab_label_button_clicked_cb(
    GtkWidget *widget, gpointer data) {
  MultiPhotoPage *photoSelectPage = (MultiPhotoPage *)data;
  if (0 != photoSelectPage) {
    photoSelectPage->quit();
  }
}

/* static */ void MultiPhotoPage::clear_button_clicked_cb(GtkWidget *widget, gpointer data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->clear_button_clicked(widget, data);
  }
}

void MultiPhotoPage::clear_button_clicked(GtkWidget *widget, gpointer data) {
  string tag_name = tag_button_map[widget];
  int index = 0;
  BOOST_FOREACH(string file_name, photoFilenameVector) {
    PhotoState &photo_state = photo_state_map[index];
    if (photo_state.get_is_selected()) {
      if (0 != all_photo_tags_for_project[file_name].count(tag_name)) {
        bool b = Db::remove_tag_by_filename_transaction(tag_name, file_name);
        if (!b) {
          // TODO Handle Db::remove_tag_by_filename_transaction failure
        }
      }
    }
    index++;
  }
  count_tags();
  rebuild_tag_view();
}

/* static */ void MultiPhotoPage::set_button_clicked_cb(GtkWidget *widget, gpointer data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->set_button_clicked(widget, data);
  }
}

void MultiPhotoPage::set_button_clicked(GtkWidget *widget, gpointer data) {
  string tag_name = tag_button_map[widget];
  int index = 0;
  BOOST_FOREACH(string file_name, photoFilenameVector) {
    PhotoState &photo_state = photo_state_map[index];
    if (photo_state.get_is_selected()) {
      if (0 == all_photo_tags_for_project[file_name].count(tag_name)) {
        bool b = Db::add_tag_by_filename_transaction(tag_name, file_name);
        if (!b) {
          // TODO Handle Db::add_tag_by_filename_transaction failure
        }
      }
    }
    index++;
  }
  count_tags();
  rebuild_tag_view();
}

//! A hack to force the GtkIconView to re-layout. It sets it to 1 column then immediately
//! sets it back to it's correct number of columns.
/* static */ void MultiPhotoPage::icon_view_size_allocate_cb(GtkWidget *widget, GdkRectangle *allocation,
    gpointer user_data) {
  GtkIconView *icon_view = GTK_ICON_VIEW(widget);

  gint num_cols = gtk_icon_view_get_columns(GTK_ICON_VIEW(widget));
  gtk_icon_view_set_columns(GTK_ICON_VIEW(widget), 1);
  gtk_icon_view_set_columns(GTK_ICON_VIEW(widget), num_cols);
}

/* static */ gboolean MultiPhotoPage::icon_view_key_press_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage->icon_view_key_press(widget, event, user_data);
  }
  return false;
}

//! non-static callback for a keyboard key press on the GtkIconView icon_view.
//! \param widget icon_view
//! \param GdkEvent the event associated with the key press
//! \param user_data NULL
gboolean MultiPhotoPage::icon_view_key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  gboolean ret = false;
  GtkTreePath *path;
  GtkTreePath *filtered_path;
  GtkCellRenderer *cell;
  int index;

  // Find the pointer coordinates within the icon view and from that get the filtered path
  // to the icon the pointer is over.
  gint x, y;
  find_pointer_coords(widget, &x, &y);
  gboolean has_item = gtk_icon_view_get_item_at_pos(GTK_ICON_VIEW(widget), x, y,
      &filtered_path, &cell);

  // The mouse might not have been over an icon
  if (has_item) {

    // Find the unfiltered path from the filtered path
    path = gtk_tree_model_filter_convert_path_to_child_path(
        GTK_TREE_MODEL_FILTER(tree_model_filter), filtered_path);

    // If we got the unfiltered path, handle the key press
    if (path) {
      guint keyval = ((GdkEventKey *)event)->keyval;
      guint state = ((GdkEventKey *)event)->state;
      switch (keyval) {
        case 'r':
          index = gtk_tree_path_get_indices(path)[0];
          rotate(index);
          ret = true;
        default:
          break;
      }
      gtk_tree_path_free(path);
    }
    gtk_tree_path_free(filtered_path);
  }
  return ret;
}

void MultiPhotoPage::set_position(int val) {
  // TODO WRITEME
}

//! static callback for a keyboard key press on the GtkIconView icon_view.
//! \param widget icon_view
//! \param GdkEvent the event associated with the key press
//! \param user_data NULL
gboolean /* static */ MultiPhotoPage::icon_view_button_press_cb(GtkWidget *widget,
    GdkEvent *event, gpointer user_data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage->icon_view_button_press(widget, event, user_data);
  }
}

//! Object callback for a mouse button press
gboolean MultiPhotoPage::icon_view_button_press(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
    return icon_view_button_press_left(widget, event, user_data);
  } else if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
    return icon_view_button_press_right(widget, event, user_data);
  } else if (event->type == GDK_2BUTTON_PRESS && event->button.button == 1) {
    int index = find_photo_index(widget);
    open_single_photo_page(index);
  } else {
  }
  return false;
}

//! Object callback for a LEFT mouse button press
gboolean MultiPhotoPage::icon_view_button_press_left(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  int index = find_photo_index(widget);
  if (-1 != index) {
    PhotoState &photo_state = photo_state_map[index];
    GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
    GtkTreePath *filtered_path = gtk_tree_model_filter_convert_child_path_to_path(
        GTK_TREE_MODEL_FILTER(tree_model_filter), path);
    gboolean is_selected = gtk_icon_view_path_is_selected(GTK_ICON_VIEW(widget), filtered_path);
    if (is_selected) {
      gtk_icon_view_unselect_path(GTK_ICON_VIEW(widget), filtered_path);
      photo_state.set_is_selected(false);
    } else {
      gtk_icon_view_select_path(GTK_ICON_VIEW(widget), filtered_path);
      photo_state.set_is_selected(true);
    }
    gtk_tree_path_free(path);
    gtk_tree_path_free(filtered_path);
    current_index = index;
    count_tags();
    rebuild_tag_view();
    rebuild_exif_view();
    // make sure that the icon_view retains the keyboard focus (the rebuilds sometimes steal it)
    gtk_grab_add(icon_view);
  }
  return TRUE;
}

//! Object callback for a RIGHT mouse button press
gboolean MultiPhotoPage::icon_view_button_press_right(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  view_icon_view_popup_menu(widget, (GdkEventButton*)event, user_data);
  return TRUE;
}

//! Find the index (index of photo in the photoFileVector) of the icon under
//! the cursor in a GtkIconView. Returns -1 if the cursor is not over an icon.
int MultiPhotoPage::find_photo_index(GtkWidget *widget) {
  gint x, y, index;
  find_pointer_coords(widget, &x, &y);
  GtkTreePath *path;
  GtkTreePath *filtered_path;
  GtkCellRenderer *cell;
  gboolean has_item = gtk_icon_view_get_item_at_pos(GTK_ICON_VIEW(widget), x, y,
      &filtered_path, &cell);
  if (has_item) {
    path = gtk_tree_model_filter_convert_path_to_child_path(
        GTK_TREE_MODEL_FILTER(tree_model_filter), filtered_path);
    index = gtk_tree_path_get_indices(path)[0];
    gtk_tree_path_free(filtered_path);
    gtk_tree_path_free(path);
  } else {
    index = -1;
  }
  return index;
}

/* static */ gboolean MultiPhotoPage::icon_view_popup_menu_cb(GtkWidget *widget, gpointer user_data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage-> icon_view_popup_menu(widget, user_data);
  }
}

gboolean MultiPhotoPage::icon_view_popup_menu(GtkWidget *widget, gpointer user_data) {
  view_icon_view_popup_menu(widget, (GdkEventButton*)0, user_data);
  return true;
}

void MultiPhotoPage::view_icon_view_popup_menu(GtkWidget *widget, GdkEventButton *event, gpointer userdata) {
  int index = find_photo_index(widget);
  if (-1 == index) {
    return;
  }
  string file_name = photoFilenameVector[index];

  GtkWidget *menu = gtk_menu_new();

  GtkWidget *menuitem0 = gtk_menu_item_new_with_label("Extend selection to here");
  g_signal_connect(menuitem0, "activate",
      (GCallback) icon_view_popup_extend_selection_activate_cb, widget);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem0);

  GtkWidget *menuitem1 = gtk_menu_item_new_with_label("Open Image Viewer");
  g_signal_connect(menuitem1, "activate",
      (GCallback) icon_view_popup_open_image_viewer_activate_cb, widget);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem1);

  GtkWidget *menuitem2 = gtk_menu_item_new_with_label("Tags");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem2);

  GtkWidget *menu2 = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem2), menu2);

  // all_photo_tags_for_project[file_name][tag_name]
  set<string> tag_map = all_photo_tags_for_project[file_name];
  typedef pair<string, Db::project_tag_s> map_entry_t;
  BOOST_FOREACH(map_entry_t map_entry, project_tags) {
    string tag_name = map_entry.first;
    GtkWidget *menuitem = gtk_check_menu_item_new_with_label(tag_name.c_str());
    g_object_set_data_full(G_OBJECT(menuitem), "file_name", new string(file_name),
        delete_string);
    if (tag_map.count(tag_name)) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), true);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu2), menuitem);
    g_signal_connect(menuitem, "toggled", (GCallback) icon_view_popup_tag_toggled_cb, widget);
  }
  g_signal_connect(menu, "deactivate", (GCallback) icon_view_popup_deactivate_cb, widget);

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0,
      gdk_event_get_time((GdkEvent*)event));
}

/* static */ void MultiPhotoPage::delete_string(void *vs) {
  string *s = (string *) vs;
  free (s);
}

/* static */ void MultiPhotoPage::icon_view_popup_extend_selection_activate_cb(GtkMenuItem *menu_item,
    gpointer user_data) {
  GtkWidget *widget = (GtkWidget *)user_data;
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->icon_view_popup_extend_selection_activate(menu_item, user_data);
  }
}

/* static */ void MultiPhotoPage::icon_view_popup_open_image_viewer_activate_cb(GtkMenuItem *menu_item,
    gpointer user_data) {
  GtkWidget *widget = (GtkWidget *)user_data;
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->icon_view_popup_open_image_viewer_activate(menu_item, user_data);
  }
}

//! Implements the Extend selection to here popup menu entry
//! finds the index of the thumbnail where the popup was activated, then searches
//! backwards for the first visible selected thumbnail. Then selects all the visible
//! thumbnails in that range.
void MultiPhotoPage::icon_view_popup_extend_selection_activate(GtkMenuItem *menu_item, gpointer user_data) {
  GtkWidget *widget = (GtkWidget*) user_data;

  // Find out the index of the thumbnail where the popup was activated.
  int end_index = find_photo_index(widget);

  // don't proceed if we didn't find the index of an icon
  if (-1 == end_index) return;

  // Find out the greatest index of a preceeding visible icon. Use -1 if none found
  int start_index;
  for (start_index = end_index - 1; start_index >= 0; start_index--) {
    if (is_visible_thumbnail(start_index)) {
      if (photo_state_map[start_index].get_is_selected()) {
        break;
      }
    }
  }

  // If there is no preceeding selected visible icon, start at 0
  if (-1 == start_index) {
    start_index = 0;
  }

  for (int index = start_index; index <= end_index; index++) {
    if (is_visible_thumbnail(index)) {
      select_thumbnail_by_index(index, true);
    }
  }
}

bool MultiPhotoPage::is_visible_thumbnail(int index) {
  // indexes that are out of bounds are considered not visible
  int num_photo_files = photoFilenameVector.size();
  if (index < 0 || index >= num_photo_files) {
    return false;
  }
  // Get a GtkTreePath based for the index in the unfiltered tree model
  GtkTreePath *path =
      gtk_tree_path_new_from_string(boost::lexical_cast<string>(index).c_str());
  // From that, get a GtkTreePath to it in the filtered tree mode.
  GtkTreePath *filtered_path = gtk_tree_model_filter_convert_child_path_to_path(
      GTK_TREE_MODEL_FILTER(tree_model_filter), path);
  gtk_tree_path_free(path);
  // If there's not filtered_path, that means it's not visible
  if (!filtered_path) {
    return false;
  }
  gtk_tree_path_free(filtered_path);
  return true;
}

void MultiPhotoPage::icon_view_popup_open_image_viewer_activate(GtkMenuItem *menu_item, gpointer user_data) {
  string label = gtk_menu_item_get_label(menu_item);
  GtkWidget *widget = (GtkWidget*) user_data;
  int index = find_photo_index(widget);
  if (-1 != index) {
    open_single_photo_page(index);
  }
}

/* static */ void MultiPhotoPage::icon_view_popup_deactivate_cb(GtkMenuShell *menu_item, gpointer user_data) {
  g_object_ref_sink(G_OBJECT(menu_item));
  g_object_unref(G_OBJECT(menu_item));
}
  
/* static */ void MultiPhotoPage::icon_view_popup_tag_toggled_cb(GtkMenuItem *menu_item, gpointer user_data) {
  GtkWidget *widget = (GtkWidget *)user_data;
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    photoSelectPage->icon_view_popup_tag_toggled(menu_item, user_data);
  }
}

void MultiPhotoPage::icon_view_popup_tag_toggled(GtkMenuItem *menu_item, gpointer user_data) {
  string tag_name = gtk_menu_item_get_label(menu_item);
  GtkWidget *widget = (GtkWidget*) user_data;
  string file_name(*(string *)g_object_get_data(G_OBJECT(menu_item), "file_name"));
  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
    bool b = Db::add_tag_by_filename_transaction(tag_name, file_name);
    if (!b) {
      // TODO Handle Db::add_tag_by_filename_transaction failure
    }
  } else {
    bool b = Db::remove_tag_by_filename_transaction(tag_name, file_name);
    if (!b) {
      // TODO Handle Db::remove_tag_by_filename_transaction failure
    }
  }
  count_tags();
  rebuild_tag_view();
}

/* static */ void MultiPhotoPage::find_pointer_coords(GtkWidget *widget, gint *x, gint *y) {
  Utils::get_pointer(widget, x, y);
  *x += gtk_adjustment_get_value(gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widget)));
  *y += gtk_adjustment_get_value(gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widget)));
}

/* static */ gboolean
MultiPhotoPage::icon_view_enter_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage-> icon_view_enter(widget, event, user_data);
  }
} 

//! grab the focus when the GtkIconView is entered. This lets it get keyboard events.
//! The grab is removed in icon_view_leave()
gboolean MultiPhotoPage::icon_view_enter(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  gtk_grab_add(widget);
}

/* static */ gboolean
MultiPhotoPage::icon_view_leave_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  MultiPhotoPage *photoSelectPage =
      (MultiPhotoPage *) WidgetRegistry<PhotoSelectPage>::get_object(widget);
  if (0 != photoSelectPage) {
    return photoSelectPage-> icon_view_leave(widget, event, user_data);
  }
} 

//! un-grab the focus when the GtkIconView is left. This lets it get keyboard events.
//! Focus is  grabbed in icon_view_enter()
gboolean MultiPhotoPage::icon_view_leave(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  gtk_grab_remove(widget);
} 

string MultiPhotoPage::get_photofile_name(int index) const {
  return photoFilenameVector[index];
}

/// Add thumbnails to the work list.
/// \param first the index into tree_model_filter of the first icon to add
/// \param last the index into tree_model_filter of the last icon to add
void MultiPhotoPage::refresh_thumbnails(int first, int last) {
  boost::lock_guard<boost::mutex> member_lock(class_mutex); 
  cout << "refresh_thumbnails " << first << " " << last << endl;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  long priority = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
  for (int i = last; i >= first; i--) {
    // convert from an index into tree_model_filter to an index into tree_model
    int tree_model_index = tree_model_filter_index_to_tree_model_index(i);
cout << "visible " << i << " " << tree_model_index << endl;
    PhotoState &photo_state = photo_state_map[tree_model_index];

    if (photo_state.get_pixbuf() == gtk_stock_missing_image) {
      int rotation;
      Db::get_rotation_transaction(photoFilenameVector[tree_model_index], rotation);
      WorkItem work_item(ticket_number, tree_model_index, rotation, priority, this);
      work_list.add_work(work_item);
    }
  }
}

/// Convert an index in the tree_model_filter to an index in the tree_model
/// \param tree_model_filter_index The index in the tree_model_filter
/// \return The index in the tree_model
int MultiPhotoPage::tree_model_filter_index_to_tree_model_index(int tree_model_filter_index) {
  GtkTreePath *filter_path = gtk_tree_path_new_from_indices(tree_model_filter_index, -1);
  GtkTreePath *child_path =
      gtk_tree_model_filter_convert_path_to_child_path(GTK_TREE_MODEL_FILTER(tree_model_filter),
      filter_path);
  int tree_model_index = atoi(gtk_tree_path_to_string(child_path));
  gtk_tree_path_free(filter_path);
  gtk_tree_path_free(child_path);
  return tree_model_index;
}

/* static */ gboolean MultiPhotoPage::tree_model_filter_func_cb(GtkTreeModel *model,
    GtkTreeIter *unfiltered_iter, gpointer data) {
  MultiPhotoPage *multiPhotoPage = (MultiPhotoPage *)data;
  return multiPhotoPage->tree_model_filter_func(model, unfiltered_iter, data);
}

gboolean MultiPhotoPage::tree_model_filter_func(GtkTreeModel *model, GtkTreeIter *unfiltered_iter,
    gpointer data) {
  gboolean ret = TRUE;
  GtkTreePath *path = gtk_tree_model_get_path(model, unfiltered_iter);
  int index = gtk_tree_path_get_indices(path)[0];
  string file_path = photoFilenameVector[index];
  cout << "Filtering " << index << " " << file_path << endl;

  if (view_filter_show_all) {
    return true;
  }

  if (view_filter_show) {
    // view/limit by tags/show these tags was set in the menu
    if (all_photo_tags_for_project.count(file_path)) {
      // this file has tags, so scan through the required tags and make sure that we have them
      set<string> tags_for_this_file_path = all_photo_tags_for_project[file_path];
      BOOST_FOREACH(string required_tag, view_filter_show_tags) {
        if (0 == tags_for_this_file_path.count(required_tag)) {
          // this file is missing a required tag, so don't show it
          ret = false;
          break;
        }
      }
    } else {
      // This file has no tags, so don't show it
      ret = false;
    }
  } else {
    BOOST_ASSERT(view_filter_dont_show);
    // view/limit by tags/dont show these tags was set in the menu
    if (all_photo_tags_for_project.count(file_path)) {
      // this file has tags, so scan through the prohibited tags. make sure that we dont have them
      set<string> tags_for_this_file_path = all_photo_tags_for_project[file_path];
      BOOST_FOREACH(string prohibited_tag, view_filter_dont_show_tags) {
        if (0 != tags_for_this_file_path.count(prohibited_tag)) {
          // this file has a prohibited tag, so don't show it
          ret = false;
          break;
        }
      }
    } else {
      // This file has no tags, so show it
    }
  }
  return ret;
}

/* static */ void MultiPhotoPage::show_menu_item_activate_cb(GtkMenuItem *menuitem, gpointer user_data) {
  cout << "show_menu_item_activate_cb " << gtk_menu_item_get_label(menuitem) << endl;
  MultiPhotoPage *photoSelectPage = (MultiPhotoPage *) user_data;
  if (0 != photoSelectPage) {
    photoSelectPage->show_menu_item_activate(menuitem, user_data);
  }
}

void MultiPhotoPage::show_menu_item_activate(GtkMenuItem *menuitem, gpointer user_data) {
  change_view_filtering();
}

/* static */ void MultiPhotoPage::show_tag_menu_item_activate_cb(GtkMenuItem *menuitem, gpointer user_data) {
  cout << "show_tag_menu_item_activate_cb " << gtk_menu_item_get_label(menuitem) << endl;
  MultiPhotoPage *photoSelectPage = (MultiPhotoPage *) user_data;
  if (0 != photoSelectPage) {
    photoSelectPage->show_tag_menu_item_activate(menuitem, user_data);
  }
}

void MultiPhotoPage::show_tag_menu_item_activate(GtkMenuItem *menuitem, gpointer user_data) {
  change_view_filtering();
}

void MultiPhotoPage::change_view_filtering() {
  cout << "change_view_filtering " << endl;

  // Look at the menu items to see what we are supposed to show.
  view_filter_show_all = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(show_all_menu_item));
  view_filter_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(
      show_these_tags_menu_item));
  view_filter_dont_show =  gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(
      dont_show_these_tags_menu_item));

  // Make a list of tags that should be shown when view_filter_show is set.
  view_filter_show_tags.clear();
  BOOST_FOREACH (GtkWidget *item, view_filter_show_menu_items) {
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
      view_filter_show_tags.insert(gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
    }
  }

  // Make a list of tags that should be shown when view_filter_dont_show is set.
  view_filter_dont_show_tags.clear();
  BOOST_FOREACH (GtkWidget *item, view_filter_dont_show_menu_items) {
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
      view_filter_dont_show_tags.insert(gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
    }
  }

  // Now we can refilter the icons.
  gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(tree_model_filter));
}

#include "BaseWindow.h"
#include "SinglePhotoPage.h"

//! Called by BaseWindow when this page is switched to. It's job is to load the menu items
//! that this page needs into the base window. It creates a vector of ExtraMenuItems\
//! and calls BaseWindow::add_extra_menu_items with them.
void MultiPhotoPage::load_extra_menu_items() {
  vector<BaseWindow::ExtraMenuItem> extra_menu_items;
  // make extra menu items
  BaseWindow::ExtraMenuItem extra_menu_item;
  extra_menu_item.item_location = "View";

  // Make the Limit by tags menu item and menu
  GtkWidget *view_limit_menu_item = gtk_menu_item_new_with_label("Limit by tags");
  extra_menu_item.menu_item = view_limit_menu_item;
  gtk_widget_show(view_limit_menu_item);

  GtkWidget *view_limit_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_limit_menu_item), view_limit_menu);
  gtk_widget_show(view_limit_menu);

  // Make the Show all menu item
  show_all_menu_item = gtk_radio_menu_item_new_with_label(NULL, "Show all");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_all_menu_item), view_filter_show_all);
  g_signal_connect(show_all_menu_item, "activate",
      G_CALLBACK(show_menu_item_activate_cb), gpointer(this));
  gtk_widget_show(show_all_menu_item);
  gtk_container_add(GTK_CONTAINER(view_limit_menu), show_all_menu_item);

  // Make the Show these tags menu item and menu
  show_these_tags_menu_item =
      gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(show_all_menu_item),
      "Show these tags");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_these_tags_menu_item),
      view_filter_show);
  g_signal_connect(show_these_tags_menu_item, "activate",
      G_CALLBACK(show_menu_item_activate_cb), gpointer(this));
  gtk_widget_show(show_these_tags_menu_item);
  gtk_container_add(GTK_CONTAINER(view_limit_menu), show_these_tags_menu_item);
  GtkWidget *show_these_tags_menu = gtk_menu_new();
  gtk_widget_show(show_these_tags_menu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(show_these_tags_menu_item), show_these_tags_menu);

  // Make the Don't show these tags menu item and menu
  dont_show_these_tags_menu_item =
      gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(show_all_menu_item),
      "Don't show these tags");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(dont_show_these_tags_menu_item),
      view_filter_dont_show);
  g_signal_connect(dont_show_these_tags_menu_item, "activate",
      G_CALLBACK(show_menu_item_activate_cb), gpointer(this));
  gtk_widget_show(dont_show_these_tags_menu_item);
  gtk_container_add(GTK_CONTAINER(view_limit_menu), dont_show_these_tags_menu_item);
  GtkWidget *dont_show_these_tags_menu = gtk_menu_new();
  gtk_widget_show(show_these_tags_menu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(dont_show_these_tags_menu_item),
      dont_show_these_tags_menu);

  // Get all the tags for this project
  typedef pair<string, Db::project_tag_s> map_entry_t;
  // (project_tags[tag_name] -> project_tag_s. (project_tag_s is empty))
  view_filter_show_menu_items.clear();
  view_filter_dont_show_menu_items.clear();
  BOOST_FOREACH(map_entry_t map_entry, project_tags) {
    string tag_name = map_entry.first;

    GtkWidget *show_tag_menu_item = gtk_check_menu_item_new_with_label(tag_name.c_str());
    g_signal_connect(show_tag_menu_item, "activate",
        G_CALLBACK(show_tag_menu_item_activate_cb), gpointer(this));
    gtk_widget_show(show_tag_menu_item);
    gtk_container_add(GTK_CONTAINER(show_these_tags_menu), show_tag_menu_item);
    view_filter_show_menu_items.push_back(show_tag_menu_item);

    GtkWidget *dont_show_tag_menu_item = gtk_check_menu_item_new_with_label(tag_name.c_str());
    g_signal_connect(dont_show_tag_menu_item, "activate",
        G_CALLBACK(show_tag_menu_item_activate_cb), gpointer(this));
    gtk_widget_show(dont_show_tag_menu_item);
    gtk_container_add(GTK_CONTAINER(dont_show_these_tags_menu), dont_show_tag_menu_item);
    view_filter_dont_show_menu_items.push_back(dont_show_tag_menu_item);
  }

  extra_menu_items.push_back(extra_menu_item);
    
  BaseWindow *baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(page_hbox));
  if (NULL != baseWindow) {
    baseWindow->add_extra_menu_items(extra_menu_items);
  };
  change_view_filtering();
}

void MultiPhotoPage::quit() {
  BaseWindow *baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(page_hbox));
  if (NULL != baseWindow) {
    baseWindow->remove_page(page_hbox);
  };
  delete this;
}

void MultiPhotoPage::add_page_to_base_window(PhotoSelectPage *photo_page) {
  BaseWindow *baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(page_hbox));
  if (NULL != baseWindow) {
    baseWindow->add_page(photo_page->get_tab_label(),
        photo_page->get_notebook_page(), project_name);
  }
};

void MultiPhotoPage::open_single_photo_page(int index) {
    SinglePhotoPage *single_photo_page = new SinglePhotoPage(photoFileCache);
    single_photo_page->setup(photoFilenameVector, adjusted_date_time_vector,
        project_name, thePreferences);
    single_photo_page->set_position(index+1); // (set_position is 1-based)
    add_page_to_base_window(single_photo_page);
}

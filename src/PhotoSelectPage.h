#ifndef PHOTOSELECTPAGE_H__
#define PHOTOSELECTPAGE_H__

//!
//! Virtual Class that represents an interface for a page in the PhotoSelect notebook.
class PhotoSelectPage {
  public:
    virtual PhotoSelectPage *clone() = 0;
    virtual GtkWidget * get_notebook_page() = 0;
    virtual const std::string &get_project_name() = 0;
    virtual GtkWidget * get_tab_label() = 0;
    virtual void rebuild_tag_view() = 0;
    virtual void rebuild_exif_view() = 0;
    virtual void set_exifs_position(const std::string position) = 0;
    virtual void set_tags_position(const std::string position) = 0;
    virtual void quit() = 0;
};
#endif  // PHOTOSELECTPAGE_H__

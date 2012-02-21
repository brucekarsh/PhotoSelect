#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include "PhotoSelectWindow.h"

main(int argc, char **argv)
{
  gtk_init(&argc, &argv);
  PhotoSelectWindow photoSelectWindow1;
  PhotoSelectWindow photoSelectWindow2;
  list<string> photoFilenameList1;
  list<string> photoFilenameList2;

  photoFilenameList1.push_back("/home/bruce/Tanzania2012/AW100/DSCN0651.JPG");
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/AW100/DSCN0551.JPG");
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/D7000-6/DSC_8557.JPG");
  photoFilenameList2.push_back("/home/bruce/Tanzania2012/AW100/DSCN0351.JPG");
  photoFilenameList2.push_back("/home/bruce/Tanzania2012/AW100/DSCN0451.JPG");

  photoSelectWindow1.setup(photoFilenameList1);
  photoSelectWindow2.setup(photoFilenameList2);
  gtk_main();
}

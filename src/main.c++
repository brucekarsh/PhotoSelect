#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include "PhotoSelectWindow.h"

main(int argc, char **argv)
{
  gtk_init(&argc, &argv);

  list<string> photoFilenameList1;
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/AW100/DSCN0651.JPG");
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/AW100/DSCN0551.JPG");
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/D7000-6/DSC_8557.JPG");
  PhotoSelectWindow photoSelectWindow1;
  photoSelectWindow1.setup(photoFilenameList1);

  //list<string> photoFilenameList2;
  //photoFilenameList2.push_back("/home/bruce/Tanzania2012/AW100/DSCN0351.JPG");
  //photoFilenameList2.push_back("/home/bruce/Tanzania2012/AW100/DSCN0451.JPG");
  //PhotoSelectWindow photoSelectWindow2;
  //photoSelectWindow2.setup(photoFilenameList2);
  gtk_main();
}

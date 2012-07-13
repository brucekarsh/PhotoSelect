#include "PageRegistry.h"
#include <map>

using namespace std;

map<GtkWidget*, PhotoSelectPage*> PageRegistry<PhotoSelectPage>::pageMap;

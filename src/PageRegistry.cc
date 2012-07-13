#include "PageRegistry.h"
#include <map>

using namespace std;

class PhotoSelectPage;

template <class T> std::map<GtkWidget*, T*> PageRegistry<T>::pageMap;

template class PageRegistry<PhotoSelectPage>;


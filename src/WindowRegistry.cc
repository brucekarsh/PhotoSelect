#include "WindowRegistry.h"
#include <map>

using namespace std;

class BaseWindow;
class DeleteProjectWindow;
class ImportWindow;
class NewProjectWindow;
class OpenProjectWindow;
class PreferencesWindow;
class QueryWindow;
class RenameProjectWindow;

template<class T>  std::map<GtkWidget*, T*> WindowRegistry<T>::windowMap;

template class WindowRegistry<BaseWindow>;
template class WindowRegistry<DeleteProjectWindow>;
template class WindowRegistry<ImportWindow>;
template class WindowRegistry<NewProjectWindow>;
template class WindowRegistry<OpenProjectWindow>;
template class WindowRegistry<PreferencesWindow>;
template class WindowRegistry<QueryWindow>;
template class WindowRegistry<RenameProjectWindow>;

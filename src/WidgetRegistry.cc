#include "WidgetRegistry.h"
#include <map>

using namespace std;

class BaseWindow;
class DeleteProjectWindow;
class EditTagsWindow;
class ImportWindow;
class NewProjectWindow;
class AddToProjectWindow;
class RemoveFromProjectWindow;
class OpenProjectWindow;
class PreferencesWindow;
class RenameProjectWindow;
class PhotoSelectPage;
class QueryView;

template<class T>  std::map<GtkWidget*, T*> WidgetRegistry<T>::widget_map;

template class WidgetRegistry<BaseWindow>;
template class WidgetRegistry<DeleteProjectWindow>;
template class WidgetRegistry<EditTagsWindow>;
template class WidgetRegistry<ImportWindow>;
template class WidgetRegistry<NewProjectWindow>;
template class WidgetRegistry<AddToProjectWindow>;
template class WidgetRegistry<RemoveFromProjectWindow>;
template class WidgetRegistry<OpenProjectWindow>;
template class WidgetRegistry<PreferencesWindow>;
template class WidgetRegistry<RenameProjectWindow>;
template class WidgetRegistry<PhotoSelectPage>;
template class WidgetRegistry<QueryView>;


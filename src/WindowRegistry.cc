#include "WindowRegistry.h"
#include <map>

using namespace std;

map<GtkWindow*, PhotoSelectWindow*> WindowRegistry::photoSelectWindowMap;
map<GtkWindow*, PreferencesWindow*> WindowRegistry::preferencesWindowMap;
map<GtkWindow*, ImportWindow*> WindowRegistry::importWindowMap;
map<GtkWindow*, QueryWindow*> WindowRegistry::queryWindowMap;

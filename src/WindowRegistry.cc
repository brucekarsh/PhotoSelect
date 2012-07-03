#include "WindowRegistry.h"
#include <map>

using namespace std;

map<GtkWidget*, PhotoSelectPage*> WindowRegistry::photoSelectPageMap;
map<GtkWindow*, PreferencesWindow*> WindowRegistry::preferencesWindowMap;
map<GtkWindow*, ImportWindow*> WindowRegistry::importWindowMap;
map<GtkWindow*, QueryWindow*> WindowRegistry::queryWindowMap;

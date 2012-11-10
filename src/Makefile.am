bin_PROGRAMS = PhotoSelect
PhotoSelect_SOURCES = main.cc \
    AddToProjectWindow.cc \
    BaseWindow.cc \
    ConversionEngine.cc \
    DeleteProjectWindow.cc \
    EditTagsWindow.cc \
    ImportWindow.cc \
    MultiPhotoPage.cc \
    NewProjectWindow.cc \
    OpenProjectWindow.cc \
    Preferences.cc \
    PreferencesWindow.cc \
    QueryView.cc \
    SinglePhotoPage.cc \
    WidgetRegistry.cc

# Note. The version of json_spirit installed on ubuntu lead to linker errors when building
# PhotoSelect. So I had to build a copy from scratch and replace the one in /usr/lib. I used v4.04.
#  bruce
AM_CPPFLAGS = -std=gnu++0x $(PhotoSelect_CFLAGS) $(LIBCRYPTO_CFLAGS) -I/usr/include/cppconn -I/opt/local/include/cppconn
PhotoSelect_LDADD = $(PhotoSelect_LIBS) $(LIBCRYPTO_LIBS) -ljpeg -lboost_program_options -lboost_regex -lboost_thread -lmysqlcppconn -ljson_spirit

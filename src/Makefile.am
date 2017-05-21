bin_PROGRAMS = PhotoSelect
PhotoSelect_SOURCES = main.cc \
    AddToProjectWindow.cc \
    AdjustTimeWindow.cc \
    BaseWindow.cc \
    ConversionEngine.cc \
    ConvertedPhotoFile.cc \
    Db.cc \
    DeleteProjectWindow.cc \
    EditTagsWindow.cc \
    ExportProjectWindow.cc \
    ImportWindow.cc \
    MultiPhotoPage.cc \
    NewProjectWindow.cc \
    OpenProjectWindow.cc \
    PhotoDbImporter.cc \
    PhotoFileCache.cc \
    Preferences.cc \
    PreferencesWindow.cc \
    QueryView.cc \
    RemoveFromProjectWindow.cc \
    RenameProjectWindow.cc \
    SinglePhotoPage.cc \
    StockThumbnails.cc \
    WidgetRegistry.cc \
    Worker.cc \
    WorkList.cc

# Note. The version of json_spirit installed on ubuntu led to linker errors when building
# PhotoSelect. So I had to build a copy from scratch and replace the one in /usr/lib. I used v4.04.
#  bruce
AM_CPPFLAGS = -I/usr/include -std=gnu++0x $(PhotoSelect_CFLAGS) $(LIBCRYPTO_CFLAGS) -I/usr/include/cppconn -I/opt/local/include/cppconn -Wall
PhotoSelect_LDADD = $(PhotoSelect_LIBS) $(LIBCRYPTO_LIBS) -ljson_spirit -ljpeg -lboost_program_options -lboost_regex -lboost_thread -lboost_system -lmysqlcppconn

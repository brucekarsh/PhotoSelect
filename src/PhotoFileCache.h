#ifndef PHOTOFILECACHE_H__
#define PHOTOFILECACHE_H__

#include <map>
#include <string>

class ConvertedPhotoFile;

// TODO This needs to be MP-Safe

class PhotoFileCache {
  private:
    std::map<std::string, ConvertedPhotoFile *> map_path_to_file;
    std::map<std::string, long> map_path_to_sequence_number;
    std::map<long, std::string> map_sequence_number_to_path;
    static const int CACHESIZE = 6;
    long sequence_number;
  public:
    PhotoFileCache();
    ~PhotoFileCache();
    ConvertedPhotoFile * get(std::string photoFilePath);
    ConvertedPhotoFile * add(std::string photoFilePath);
};
#endif  // PHOTOFILECACHE_H__

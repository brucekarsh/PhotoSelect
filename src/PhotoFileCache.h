#ifndef PHOTOFILECACHE_H__
#define PHOTOFILECACHE_H__

#include <map>
#include <string>

// TODO This needs to be MP-Safe

class PhotoFileCache {
  std::map<std::string, ConvertedPhotoFile *> map_path_to_file;
  std::map<std::string, long> map_path_to_sequence_number;
  std::map<long, std::string> map_sequence_number_to_path;
  static const int CACHESIZE = 6;
  long sequence_number;

  public:
  PhotoFileCache() : sequence_number(0) {};
  ~PhotoFileCache() {
    for (
        std::map<std::string, ConvertedPhotoFile *>::iterator it = map_path_to_file.begin();
        it != map_path_to_file.end();
        ++it) {
      delete it->second;
    }
    
  }

  ConvertedPhotoFile * get(std::string photoFilePath) { 
    ConvertedPhotoFile *convertedPhotoFile = 0;
    ++sequence_number;
    // Try to get the ConvertePhotoFile from the cache
    std::map<std::string, ConvertedPhotoFile *>::iterator convertedPhotoFileIterator =
      map_path_to_file.find(photoFilePath);
    if (convertedPhotoFileIterator == map_path_to_file.end()) {
      // It was not in the cache. Read the file and add it to the cache.
      convertedPhotoFile = new ConvertedPhotoFile(photoFilePath);
      map_path_to_file.insert(
          std::pair<std::string, ConvertedPhotoFile *>(photoFilePath, convertedPhotoFile));
      map_path_to_sequence_number[photoFilePath] = sequence_number;
      map_sequence_number_to_path[sequence_number] = photoFilePath;
      // If the cache is full, remove the LRU entries
      while (map_sequence_number_to_path.size() >= CACHESIZE) {
        std::map<long, std::string>::iterator it = map_sequence_number_to_path.begin();
        long lru_sequence_number = it->first;
        std::string lru_path = it->second;
        ConvertedPhotoFile *lru_converted_photo_file = map_path_to_file[lru_path];
        map_path_to_file.erase(lru_path);
        map_path_to_sequence_number.erase(lru_path);
        map_sequence_number_to_path.erase(lru_sequence_number);
        delete lru_converted_photo_file;
      }
    } else {
      // It was in the cache. Update its sequence number
      convertedPhotoFile = convertedPhotoFileIterator->second;
      long old_sequence_number = map_path_to_sequence_number[photoFilePath];
      map_sequence_number_to_path.erase(old_sequence_number);
      map_path_to_sequence_number[photoFilePath] = sequence_number;
      map_sequence_number_to_path[sequence_number] = photoFilePath;
    }
    return convertedPhotoFile;
  }

  ConvertedPhotoFile * add(std::string photoFilePath) {
    // TODO WRITEME
    return get(photoFilePath);
  }
};
#endif  // PHOTOFILECACHE_H__

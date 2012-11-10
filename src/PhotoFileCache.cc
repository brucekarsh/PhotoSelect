#include "PhotoFileCache.h"

#include "ConvertedPhotoFile.h"

PhotoFileCache::PhotoFileCache() : sequence_number(0) {};

PhotoFileCache::~PhotoFileCache() {
  for (
      std::map<std::string, ConvertedPhotoFile *>::iterator it = map_path_to_file.begin();
      it != map_path_to_file.end();
      ++it) {
    delete it->second;
  }
}

ConvertedPhotoFile * PhotoFileCache::get(std::string photoFilePath) { 
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

ConvertedPhotoFile * PhotoFileCache::add(std::string photoFilePath) {
  // TODO WRITEME
  return get(photoFilePath);
}

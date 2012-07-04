#ifndef PHOTOFILECACHE_H__
#define PHOTOFILECACHE_H__

#include <map>
#include <string>

// TODO This needs to be MP-Safe

class PhotoFileCache {

  std::map<std::string, ConvertedPhotoFile *> photoFileCacheMap;
  public:

  ConvertedPhotoFile * get(std::string photoFilePath) { 
    ConvertedPhotoFile * convertedPhotoFile = 0;

    std::map<std::string, ConvertedPhotoFile *>::iterator convertedPhotoFileIterator = photoFileCacheMap.find(photoFilePath);
    if (convertedPhotoFileIterator == photoFileCacheMap.end()) {
      convertedPhotoFile = new ConvertedPhotoFile(photoFilePath);
      photoFileCacheMap.insert(std::pair<std::string, ConvertedPhotoFile *>(photoFilePath, convertedPhotoFile));
    } else {
      convertedPhotoFile = convertedPhotoFileIterator->second;
    }
    return convertedPhotoFile;
  }

  ConvertedPhotoFile * add(std::string photoFilePath) {
    // TODO WRITEME
    return get(photoFilePath);
  }
};
#endif  // PHOTOFILECACHE_H__

#ifndef PHOTOFILECACHE_H__
#define PHOTOFILECACHE_H__

#include <map>
#include <string>

using namespace std;

// XXX This needs to be MP-Safe

class PhotoFileCache {

  map<string, ConvertedPhotoFile *> photoFileCacheMap;
  public:

  ConvertedPhotoFile * get(string photoFilePath) { 
    ConvertedPhotoFile * convertedPhotoFile = 0;

    map<string, ConvertedPhotoFile *>::iterator convertedPhotoFileIterator = photoFileCacheMap.find(photoFilePath);
    if (convertedPhotoFileIterator == photoFileCacheMap.end()) {
      convertedPhotoFile = new ConvertedPhotoFile(photoFilePath);
      photoFileCacheMap.insert(pair<string, ConvertedPhotoFile *>(photoFilePath, convertedPhotoFile));
    } else {
      convertedPhotoFile = convertedPhotoFileIterator->second;
    }
    return convertedPhotoFile;
  }

  ConvertedPhotoFile * add(string photoFilePath) {
    // XXX WRITEME
    return get(photoFilePath);
  }
};
#endif  // PHOTOFILECACHE_H__

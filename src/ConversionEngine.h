#ifndef CONVERSIONENGINE_H__
#define CONVERSIONENGINE_H__

#include <vector>
#include <string>
#include <stdio.h>
#include "ConvertedPhotoFile.h"
#include "PhotoFileCache.h"

using namespace std;

//! Maintains a speculative cache of ConvertedPhotoFiles. It has a list of PhotoFiles and a current 
//! position in the list.  It tries to cache a few ConvertedPhotoFiles before and after the
//! current position

class ConversionEngine {
  public:
  PhotoFileCache photoFileCache;
  vector<string> photoFilenameVector;
  int photoFilenameVectorPosition;


  //! Gets a ConvertedPhotoFile given a path to a file representing a photo.
  //! It first tries to get the ConvertedPhotoFile from the cache. If it
  //! can't, t tries to find it in the photoFileMap and add it to the cache.
  //! If it can't do that either, it returns an UnknownConvertedPhotoFile.
  //! \param photoFilePath the filesystem path of the photo file for which a ConvertedPhotoFile is
  //!        requested
  /// \return A convertedPhotoFile (which could still be converting)
  ///         or an UnknownCovertedPhotoFile if it cannot convert the file.
  ConvertedPhotoFile * getConvertedPhotoFile() {
    string photoFilename = photoFilenameVector[photoFilenameVectorPosition];
    ConvertedPhotoFile * convertedPhotoFile = photoFileCache.get(photoFilename);
    if (0 == convertedPhotoFile) {
      convertedPhotoFile = photoFileCache.add(photoFilename);
    }
    return convertedPhotoFile;
  }

  void setPhotoFileList(list<string> *photoFileNameList) {
    // XXX WRITEME
    int i;
    list<string>::iterator photoFileNameListIterator = photoFileNameList->begin();
    for (i=0; i<photoFileNameList->size(); i++) {
      printf("%d %s\n", i, photoFileNameListIterator->c_str());
      photoFilenameVector.push_back(photoFileNameListIterator->c_str());
      photoFileNameListIterator++;
    }
    photoFilenameVectorPosition = 0;
  }

  void next() {
    ++photoFilenameVectorPosition;
    if (photoFilenameVectorPosition >= photoFilenameVector.size()) {
      photoFilenameVectorPosition = photoFilenameVector.size() -1;
    }
    if (photoFilenameVectorPosition < 0) {
      photoFilenameVectorPosition = 0;
    }
  }

  void back() {
    --photoFilenameVectorPosition;
    if (photoFilenameVectorPosition < 0) {
      photoFilenameVectorPosition = 0;
    }
  }

};
#endif  // CONVERSIONENGINE_H__

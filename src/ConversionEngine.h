#ifndef CONVERSIONENGINE_H__
#define CONVERSIONENGINE_H__

#include <vector>
#include <string>
#include <stdio.h>
#include "ConvertedPhotoFile.h"
#include "PhotoFileCache.h"

//! Maintains a speculative cache of ConvertedPhotoFiles. It has a list of PhotoFiles and a current 
//! position in the list.  It tries to cache a few ConvertedPhotoFiles before and after the
//! current position

class ConversionEngine {
  public:
  PhotoFileCache photoFileCache;
  std::vector<std::string> photoFilenameVector;
  int photoFilenameVectorPosition;

  ConversionEngine() : photoFilenameVectorPosition(-1) {};


  //! Gets a ConvertedPhotoFile given a path to a file representing a photo.
  //! It first tries to get the ConvertedPhotoFile from the cache. If it
  //! can't, t tries to find it in the photoFileMap and add it to the cache.
  //! If it can't do that either, it returns an UnknownConvertedPhotoFile.
  //! \param photoFilePath the filesystem path of the photo file for which a ConvertedPhotoFile is
  //!        requested
  /// \return A convertedPhotoFile (which could still be converting)
  ///         or an UnknownCovertedPhotoFile if it cannot convert the file.
  ConvertedPhotoFile * getConvertedPhotoFile() {
    if (photoFilenameVectorPosition <0
        || photoFilenameVectorPosition >= photoFilenameVector.size()) {
      return NULL;
    }
    std::string photoFilename = photoFilenameVector[photoFilenameVectorPosition];
    ConvertedPhotoFile * convertedPhotoFile = photoFileCache.get(photoFilename);
    if (0 == convertedPhotoFile) {
      convertedPhotoFile = photoFileCache.add(photoFilename);
    }
    return convertedPhotoFile;
  }

  void setPhotoFileList(std::list<std::string> *photoFileNameList) {
    int i;
    std::list<std::string>::iterator photoFileNameListIterator = photoFileNameList->begin();
    for (i=0; i<photoFileNameList->size(); i++) {
      photoFilenameVector.push_back(photoFileNameListIterator->c_str());
      photoFileNameListIterator++;
    }
    photoFilenameVectorPosition = 0;
    clip_position();
  }

  /** Enforces a constraint on photoFileVectorPosition.
      It adjusts the photoFilenameVectorPosition so that it is in the range
      0 <= photoFilenameVectorPosition < photoFilenameVector.size()
      or -1 when 0 == photoFilenameVector.size() */
  void
  clip_position() {
    if (photoFilenameVectorPosition < 0) {
      photoFilenameVectorPosition = 0;
    }
    if (photoFilenameVectorPosition >= photoFilenameVector.size()) {
      photoFilenameVectorPosition = photoFilenameVector.size() -1;
    }
    if (0 == photoFilenameVector.size()) {
      photoFilenameVectorPosition = -1;
    }
  }


  void next() {
    ++photoFilenameVectorPosition;
    clip_position();
  }

  void back() {
    --photoFilenameVectorPosition;
    clip_position();
  }

  void go_to(int position) {
    photoFilenameVectorPosition = position;
    clip_position();
  }

  int get_position() {
    return photoFilenameVectorPosition;
  }

};
#endif  // CONVERSIONENGINE_H__

#include "ConversionEngine.h"

#include "ConvertedPhotoFile.h"
#include "PhotoFileCache.h"

using namespace std;

ConversionEngine::ConversionEngine(PhotoFileCache *photoFileCache_) :
    photoFilenameVectorPosition(-1), photoFileCache(photoFileCache_) {};

ConvertedPhotoFile * ConversionEngine::getConvertedPhotoFile() {
  if (photoFilenameVectorPosition <0
      || photoFilenameVectorPosition >= photoFilenameVector.size()) {
    return NULL;
  }
  string photoFilename = photoFilenameVector[photoFilenameVectorPosition];
  ConvertedPhotoFile * convertedPhotoFile = photoFileCache->get(photoFilename);
  if (0 == convertedPhotoFile) {
    convertedPhotoFile = photoFileCache->add(photoFilename);
  }
  return convertedPhotoFile;
}

ConvertedPhotoFile *ConversionEngine::getConvertedPhotoFile(int display_width,
    int display_height, int rotation) const {
  if (photoFilenameVectorPosition <0
      || photoFilenameVectorPosition >= photoFilenameVector.size()) {
    return NULL;
  }
  string photoFilename = photoFilenameVector[photoFilenameVectorPosition];
  return getConvertedPhotoFile(photoFilename, display_width, display_height, rotation);
}

string ConversionEngine::getPhotoFilePath() {
  if (photoFilenameVectorPosition <0
      || photoFilenameVectorPosition >= photoFilenameVector.size()) {
    return "";
  }
  string photoFilename = photoFilenameVector[photoFilenameVectorPosition];
  return photoFilename;
}

void ConversionEngine::setPhotoFileVector(vector<string> *photoFilenameVector_) {
  int i;
  vector<string>::iterator photoFilenameVectorIterator = photoFilenameVector_->begin();
  for (i=0; i<photoFilenameVector_->size(); i++) {
    photoFilenameVector.push_back(photoFilenameVectorIterator->c_str());
    photoFilenameVectorIterator++;
  }
  photoFilenameVectorPosition = 0;
  clip_position();
}

void ConversionEngine::clip_position() {
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

void ConversionEngine::next() {
  ++photoFilenameVectorPosition;
  clip_position();
}

void ConversionEngine::back() {
  --photoFilenameVectorPosition;
  clip_position();
}

void ConversionEngine::go_to(int position) {
  photoFilenameVectorPosition = position;
  clip_position();
}

int ConversionEngine::get_position() {
  return photoFilenameVectorPosition;
}

// Static member functions

/* static */ ConvertedPhotoFile *ConversionEngine::getConvertedPhotoFile(
    const string &photoFilename, int display_width, int display_height, int rotation) {
  ConvertedPhotoFile * convertedPhotoFile = new ConvertedPhotoFile(photoFilename,
      display_width, display_height, rotation);
  return convertedPhotoFile;
}

#ifndef CONVERSIONENGINE_H__
#define CONVERSIONENGINE_H__

#include <vector>
#include <string>
#include <stdio.h>

class ConvertedPhotoFile;
class PhotoFileCache;

//! Maintains a speculative cache of ConvertedPhotoFiles. It has a list of PhotoFiles and a current 
//! position in the list.  It tries to cache a few ConvertedPhotoFiles before and after the
//! current position

class ConversionEngine {
  public:
    PhotoFileCache *photoFileCache;
    std::vector<std::string> photoFilenameVector;
    int photoFilenameVectorPosition;

    ConversionEngine(PhotoFileCache *photoFileCache_);

    //! Gets a ConvertedPhotoFile given a path to a file representing a photo.
    //! It first tries to get the ConvertedPhotoFile from the cache. If it
    //! can't, t tries to find it in the photoFileMap and add it to the cache.
    //! If it can't do that either, it returns an UnknownConvertedPhotoFile.
    /// \return A convertedPhotoFile (which could still be converting)
    ///         or an UnknownCovertedPhotoFile if it cannot convert the file.
    ConvertedPhotoFile * getConvertedPhotoFile();

    //! Gets a ConvertedPhotoFile for a thumbnail for the file at the current position.
    //! It does not try to cache anything. It tries to scale the image, but not so much
    //! that i becomes smaller than the specified display size.
    //! 
    //! If it can't do that either, it returns an UnknownConvertedPhotoFile.
    //! \param display_width width of the display
    //! \param display_height height of the display
    /// \return A convertedPhotoFile (which could still be converting)
    ///         or an UnknownCovertedPhotoFile if it cannot convert the file.
    ConvertedPhotoFile *getConvertedPhotoFile(int display_width, int display_height,
        int rotation) const;

    //! Gets the full pathname  of the current photo file.
    std::string getPhotoFilePath();

    void setPhotoFileVector(std::vector<std::string> *photoFilenameVector_);

    /** Enforces a constraint on photoFileVectorPosition.
        It adjusts the photoFilenameVectorPosition so that it is in the range
        0 <= photoFilenameVectorPosition < photoFilenameVector.size()
        or -1 when 0 == photoFilenameVector.size() */
    void clip_position();
    void next();
    void back();
    void go_to(int position);
    int get_position();

    // Static member functions

    //! Like ConvertedPhotoFile(int, int, int), but it accepts a filename instead of 
    //! using the current position.
    //! Gets a ConvertedPhotoFile for a thumbnail given a path to a file representing a photo.
    //! It does not try to cache anything. It tries to scale the image, but not so much
    //! that i becomes smaller than the specified display size.
    //! 
    //! If it can't do that either, it returns an UnknownConvertedPhotoFile.
    //! \param photoFilename the file from which to get a thumbnail.
    //! \param display_width width of the display
    //! \param display_height height of the display
    //! \return A convertedPhotoFile (which could still be converting)
    //!         or an UnknownCovertedPhotoFile if it cannot convert the file.

    static ConvertedPhotoFile *getConvertedPhotoFile(const std::string &photoFilename,
        int display_width, int display_height, int rotation);
};
#endif  // CONVERSIONENGINE_H__

#ifndef CONVERTEDPHOTOFILE_H__
#define CONVERTEDPHOTOFILE_H__

#include <string>
#include <math.h>
extern "C" {
#include <jpeglib.h>
};

#define MINIMUM(a,b)	((a) < (b) ? (a) : (b))
#define MAXIMUM(a,b)	((a) > (b) ? (a) : (b))
#define SPACE (" ")

class ConvertedPhotoFile;

class ConvertedPhotoFile {
  private:

    class RotationMatrix {
      public:
        float r00;
        float r01;
        float r10;
        float r11;
  
        void compute_rotation_matrix(float rotation) {
          // TODO rotation is in range 0..4. It should be something more sensible;.
          // TODO this should be a constructor
          // TODO it should be a homogeneous matrix and do scaling and translation.
          float theta = -rotation * M_PI / 2.0;
          r00 =  cos(theta);
          r10 =  sin(theta);
          r01 = -sin(theta);
          r11 =  cos(theta);
        }
  
        void
        coordinate_transform(float sx0, float sy0, float dx, float dy,
            float magnification, float *px, float *py) {
          *px = (r00 * (sx0-dx) + r01 * (sy0-dy))/magnification;
          *py = (r10 * (sx0-dx) + r11 * (sy0-dy))/magnification;
        }
    };

    struct jpeg_decompress_struct cinfo;
    FILE * infile;
  public:
    std::string photoFilePath;
    int width;
    int height;
    unsigned char *pixels;
  
    //! Construct with no libjpeg scaling
    ConvertedPhotoFile(const std::string &photoFilePath);

    //! Construct with libjpeg scaling, but don't scale more than the display size
    ConvertedPhotoFile(const std::string &photoFilePath, int display_width, int display_height,
        int rotation);

    //! Sets scale_num and scale_denom to as small a ratio as possible without
    //! scaling smaller than the display size.
    void set_JPEG_scaling(int display_width, int display_height, int rotation);
    ~ConvertedPhotoFile();
    METHODDEF(void) my_error_exit (j_common_ptr cinfo_param);

    //! Opens the file, reads the header. Populates members: cinfo, infile
    //! If it cannot read the header, then it returns with infile==NULL.
    void read_JPEG_header(const char *filename);
    unsigned char *read_JPEG_pixels();

    /**
    sw, sh   The desired width and height of the output image, in pixels. This can be larger or
             smaller than the transformed input image. The output will be clipped or padded
             as necessary.
    mag      The ratio of output pixel size over input pixel size assuming square pixels
             A mag of 1 means one input pixel maps to one output pixel, a mag of
             2 means that two input pixels map to one output pixel.
    dx, dy   The displacement between (0,0) in the picture and (0,0)
             in the output, in output pixels.
    rotation The rotation of the transformed input image. In the range 0..4, where 0
             is no rotation and 4 is 360 degrees rotation. Rotations are clockwise.

    @returns a pixel map, 4 unsigned chars per pixel (r,g,b,0) of the output image.
    */

    unsigned char *
    scale_and_pan_and_rotate(int sw, int sh, float mag, float dx, float dy, float rotation);
};
#endif  // CONVERTEDPHOTOFILE_H__

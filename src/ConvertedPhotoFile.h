#ifndef CONVERTEDPHOTOFILE_H__
#define CONVERTEDPHOTOFILE_H__

#include <map>
#include <string>
#include <malloc.h>
#include <sys/time.h>
#include <math.h>
extern "C" {
#include <jpeglib.h>
};
#include "setjmp.h"
#include "ScaledImage.h"
#include <iostream>

#define MINIMUM(a,b)	((a) < (b) ? (a) : (b))
#define MAXIMUM(a,b)	((a) > (b) ? (a) : (b))
#define SPACE (" ")

class ConvertedPhotoFile;

class ConvertedPhotoFile {
  class FractionalIncrement {
    int whole, num, denom, val, frac;
  public:
    FractionalIncrement(int num_param, int denom_param) {
        whole = num_param/denom_param;
        num   = num_param%denom_param;
        denom = denom_param;
        val   = 0;
        frac  = 0;
    };
    int increment(void) {
        int inc=whole;
        frac += num;
        if(frac >= denom) {
            frac -= denom;
            inc++;
        }
        val += inc;
        return inc;
    };
    void reset(void) {
        val = 0;
        frac = 0;
    };
  };

  public:

  std::string photoFilePath;
  int width;
  int height;
  unsigned char *pixels;
  
  ConvertedPhotoFile(std::string &photoFilePath) {
    unsigned char *pixels_tmp;
    int width_tmp, height_tmp;

    this->photoFilePath = photoFilePath;

    pixels_tmp = read_JPEG_file (photoFilePath.c_str(), &width_tmp, &height_tmp);
    if(pixels_tmp) {
      pixels = pixels_tmp;
      width  = width_tmp;
      height = height_tmp;
      } else {
        // TODO WRITEME handle read_JPEG_file failure
        pixels = 0;
        width = 0;
        height = 0;
      }
  }

  ScaledImage* scale(int output_width, int output_height) {
    float mag = MINIMUM((float)output_width/width, (float)output_height/height);
    return new ScaledImage(output_width, output_height,
        scale_and_pan_and_rotate( output_width, output_height, mag, 0, 0, 0));
  }

  struct my_error_mgr {
    struct jpeg_error_mgr pub;    /* "public" fields */
    jmp_buf setjmp_buffer;        /* for return to caller */
  };

  typedef struct my_error_mgr * my_error_ptr;

  METHODDEF(void)
  my_error_exit (j_common_ptr cinfo)
  {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
  }

  unsigned char *
  read_JPEG_file (const char * filename, int *width_tmp_p, int *height_tmp_p)
  {
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;


    /* More stuff */
    FILE * infile;                /* source file */
    int row_stride;               /* physical row width in output buffer */
    unsigned char *buffer=0;
    unsigned char *bufferp=0;

    // Open the input file.

    if ((infile = fopen(filename, "rb")) == NULL) {
      std::cout << "can't open " <<  filename << std::endl;
      return 0;
    }

    // Set up error handler
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      std::cout << "Ouch!! in setjmp handler" << std::endl;
      jpeg_destroy_decompress(&cinfo);
      fclose(infile);
  //    if(buffer)free(buffer);
  //    buffer=0;
  //    return 0;
  return buffer;
    }
  
    // Create and initialize the decompressor
  
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void) jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;  // Always return RGB.
    jpeg_calc_output_dimensions(&cinfo);
    row_stride = cinfo.output_width * cinfo.output_components;
    *width_tmp_p = cinfo.output_width;
    *height_tmp_p = cinfo.output_height;
  
    // Create a buffer to hold the output image
  
    buffer = (unsigned char *)malloc(
                  cinfo.output_width*cinfo.output_height*cinfo.output_components);
    bufferp = buffer;
  
    // Decompress the image
  
    (void) jpeg_start_decompress(&cinfo);
    while (cinfo.output_scanline < cinfo.output_height) {
      /* jpeg_read_scanlines expects an array of pointers to scanlines.
       * Here the array is only one element long, but you could ask for
       * more than one scanline at a time if that's more convenient.
       */
      (void) jpeg_read_scanlines(&cinfo, &bufferp, 1);
      bufferp += row_stride;
    }
    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
  
    fclose(infile);
    return buffer;
  }

  unsigned char *scale_pixmap(int output_width, int output_height)
  {
      unsigned char *newbuf;
      const int IN_BYTES_PER_PIXEL = 3;
      const int OUT_BYTES_PER_PIXEL = 4;
      newbuf = (unsigned char *)malloc(output_width*output_height*OUT_BYTES_PER_PIXEL);
  
      unsigned char *inbufrowp, *inbufcolp;
      unsigned char *outbufp;
      FractionalIncrement colincrement(width, output_width), rowincrement(height, output_height);
      struct timeval tv0, tv1;
      int delta_sec, delta_usec;
      double delta_t;
  
      gettimeofday(&tv0,0);
      int x,y;
      outbufp = newbuf;
      inbufrowp = pixels;
      for(y=0;y<output_height;y++) {
          inbufcolp = inbufrowp;
          for(x=0;x<output_width;x++) {
              outbufp[0] = inbufcolp[2];
              outbufp[1] = inbufcolp[1];
              outbufp[2] = inbufcolp[0];
              outbufp[3] = inbufcolp[3];
              int tmp1 = colincrement.increment();
              inbufcolp += IN_BYTES_PER_PIXEL*tmp1;
              outbufp+=OUT_BYTES_PER_PIXEL;
          }
          colincrement.reset();
          int tmp2;
          tmp2 = rowincrement.increment();
          inbufrowp += IN_BYTES_PER_PIXEL*width*tmp2;
      }
  
      gettimeofday(&tv1,0);
      delta_sec = tv1.tv_sec-tv0.tv_sec;
      delta_usec = tv1.tv_usec-tv0.tv_usec;
      if(delta_usec<0) {
          delta_usec += 1000000;
          delta_sec -= 1;
      }
      delta_t = delta_sec + delta_usec/1000000.0;
      std::cout << "Scale took " << delta_t << " secs" << std::endl;
      return newbuf;
  }

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
  scale_and_pan_and_rotate(int sw, int sh, float mag, float dx, float dy, float rotation)
  {
    int sx, sy;      // indexes in screen space
    float sx0, sy0;  // coordinates in screen space
    int px, py;      // indexes in picture space
    float px0, py0;    // coordintates in picture space
    RotationMatrix rotation_matrix;
  
    unsigned char *obuf = (unsigned char *)malloc(sw*sh*4);
    unsigned char *ip = pixels; // TODO just use pixels, we don't need ip
    rotation_matrix.compute_rotation_matrix(rotation);

    // Iterate through each screen column and compute its coordinate
    for (sx = 0; sx < sw; sx++) {
      float sx0 = sx - sw/2.0 ;

      // Iterate through each screen row and compute its coordinate
      for (sy = 0; sy < sh; sy++) {
        sy0 = sy - sh/2;

        // Find the picture coordinate of (sx0, sy0)
        rotation_matrix.coordinate_transform(sx0, sy0, dx, dy, mag, &px0, &py0);

        // Find the pixel index in the image
        px = px0 + width/2.0;
        py = py0 + height/2.0;

        // Get the pixel value
        int pr, pg, pb;
        if (px < 0 || px >= width || py < 0 || py >= height) {
           pr = 0; pg = 0; pb = 0;
        } else {
           int ic = (py * width + px) * 3;
           pr = ip[ic];
           pg = ip[ic+1];
           pb = ip[ic+2];
        }
        int oc = (sy * sw + sx) * 4;
        obuf[oc] = pb;
        obuf[oc+1] = pg;
        obuf[oc+2] = pr;
        obuf[oc+3] = 0;
      }
    }
    return obuf;
  }
};

#endif  // CONVERTEDPHOTOFILE_H__

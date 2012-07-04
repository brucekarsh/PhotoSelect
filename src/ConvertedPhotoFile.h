#ifndef CONVERTEDPHOTOFILE_H__
#define CONVERTEDPHOTOFILE_H__

#include <map>
#include <string>
#include <malloc.h>
#include <sys/time.h>
extern "C" {
#include <jpeglib.h>
};
#include "setjmp.h"
#include "ScaledImage.h"

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

    printf("Converting %s\n", photoFilePath.c_str());
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

  ScaledImage* scale(int width, int height) {
    printf("ConvertedPhotoFile::scale %d %d\n", width, height);
    return new ScaledImage(width, height, scale_pixmap(width, height));
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
    printf("In my_error_exit\n");
    (*cinfo->err->output_message) (cinfo);
    printf("Leaving my_error_exit\n");
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
      fprintf(stderr, "can't open %s\n", filename);
      return 0;
    }

    // Set up error handler
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
  printf("Ouch!! in setjmp handler\n");
  //printf("brow_stride=%d,buffer=0x%x, bufferp=0x%x, delta=%d\n",row_stride,buffer,bufferp,bufferp-buffer);
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
  //printf("Col increment %d\n",tmp1);
              inbufcolp += IN_BYTES_PER_PIXEL*tmp1;
              outbufp+=OUT_BYTES_PER_PIXEL;
          }
          colincrement.reset();
          int tmp2;
          tmp2 = rowincrement.increment();
  //printf("Row increment %d\n",tmp2);
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
      printf("Scale took %f secs\n",delta_t);
      return newbuf;
  }


};

#endif  // CONVERTEDPHOTOFILE_H__

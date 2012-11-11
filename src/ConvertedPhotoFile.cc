#include "ConvertedPhotoFile.h"

#include <iostream>
extern "C" {
#include <jpeglib.h>
};
#include <math.h>
#include "setjmp.h"
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

#define MINIMUM(a,b)	((a) < (b) ? (a) : (b))
#define MAXIMUM(a,b)	((a) > (b) ? (a) : (b))
#define SPACE (" ")

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */
  jmp_buf setjmp_buffer;        /* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

ConvertedPhotoFile::ConvertedPhotoFile(const string &photoFilePath) :
    photoFilePath(photoFilePath) {
  read_JPEG_header(photoFilePath.c_str());
  if (NULL == infile) {
    pixels = 0;
    width = 0;
    height = 0;
    return;
  }
  pixels = read_JPEG_pixels();
  if (NULL == pixels) {
    width = 0;
    height = 0;
    return;
  }
  width = cinfo.output_width;
  height = cinfo.output_height;
}

ConvertedPhotoFile::ConvertedPhotoFile(const string &photoFilePath, int display_width,
    int display_height, int rotation) : photoFilePath(photoFilePath) {
  read_JPEG_header(photoFilePath.c_str());
  if (NULL == infile) {
    pixels = 0;
    width = 0;
    height = 0;
    return;
  }
  set_JPEG_scaling(display_width, display_height, rotation);
  pixels = read_JPEG_pixels();
  if(NULL == pixels) {
    // TODO WRITEME handle read_JPEG_file failure
    pixels = 0;
    width = 0;
    height = 0;
  } else {
    width = cinfo.output_width;
    height = cinfo.output_height;
  }
}

void ConvertedPhotoFile::set_JPEG_scaling(int display_width, int display_height, int rotation) {
  int denom = 8;
  int num;
  int output_width, output_height;
  if (rotation == 0 || rotation == 2) {
    output_width = cinfo.output_width;
    output_height = cinfo.output_height;
  } else {
    output_width = cinfo.output_height;
    output_height = cinfo.output_width;
  }
  for (num = 1; num < 8; num++) {
    int scaled_width = output_width * num / denom;
    int scaled_height = output_height * num / denom;
    if (scaled_width >= display_width && scaled_height >= display_height) {
      break;
    }
  }
  cinfo.scale_num = num;
  cinfo.scale_denom = denom;
  jpeg_calc_output_dimensions(&cinfo);
}

ConvertedPhotoFile::~ConvertedPhotoFile() {
  free(pixels);
}

//! Opens the file, reads the header. Populates members: cinfo, infile
//! If it cannot read the header, then it returns with infile==NULL.
void ConvertedPhotoFile::read_JPEG_header(const char *filename) {
  struct my_error_mgr jerr;

  // Open the input file.

  if ((infile = fopen(filename, "rb")) == NULL) {
    cout << "can't open " <<  filename << endl;
    return;
  }

  // Set up error handler
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    cout << "Ouch!! in setjmp handler" << endl;
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    infile = NULL;
    return;
  }

  // Create and initialize the decompressor

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  (void) jpeg_read_header(&cinfo, TRUE);
  cinfo.out_color_space = JCS_RGB;  // Always return RGB.
  jpeg_calc_output_dimensions(&cinfo);
}

unsigned char *ConvertedPhotoFile::read_JPEG_pixels() {
  struct my_error_mgr jerr;
  unsigned char *buffer = NULL;
  int row_stride;               /* physical row width in output buffer */
  unsigned char *bufferp=0;

  row_stride = cinfo.output_width * cinfo.output_components;

  // Set up error handler
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    cout << "Ouch!! in setjmp handler" << endl;
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    if(buffer)free(buffer);
    return NULL;
  }

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

unsigned char * ConvertedPhotoFile::scale_and_pan_and_rotate(int sw, int sh,
    float mag, float dx, float dy, float rotation) {
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
    sx0 = sx - sw/2.0 ;

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

// Static member functions

/* static */ void ConvertedPhotoFile::my_error_exit (j_common_ptr cinfo_param) {
  cout << "in my_error_exit" << endl;
  my_error_ptr myerr = (my_error_ptr) cinfo_param->err;
  (*cinfo_param->err->output_message) (cinfo_param);
  longjmp(myerr->setjmp_buffer, 1);
}

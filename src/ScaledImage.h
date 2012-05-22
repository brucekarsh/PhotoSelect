#ifndef SCALED_IMAGE_H__
#define SCALED_IMAGE_H__

class ScaledImage {

  public:

  int width;
  int height;
  unsigned char *pixels;
  
  ScaledImage(int width, int height, unsigned char *pixels) {
    this->width = width;
    this->height = height;
    this->pixels = pixels;
  }

  int getWidth() { return width; }
  int getHeight() { return height; }
  unsigned char * getPixels() { return pixels; }

  void rotate180(void) {
    unsigned char *src_p, *dst_p, tmp;
    int src_x, src_y;
    src_p=pixels;
    dst_p=pixels+3*(width*height-1);
    for(src_y=0;src_y<height;src_y++){
      for(src_x=0;src_x<width;src_x++) {
        if(dst_p < src_p) {
          tmp = dst_p[0];
          dst_p[0] = src_p[0];
          src_p[0] = tmp;
          tmp = dst_p[1];
          dst_p[1] = src_p[1];
          src_p[1] = tmp;
          tmp = dst_p[2];
          dst_p[2] = src_p[2];
          src_p[2] = tmp;
        }
        src_p+=3;
        dst_p-=3;
      }
    }
  }

  void rotate90(void) {
    unsigned char *src_p, *dst_p;
    int src_x, src_y, src_offset, dst_offset;
    unsigned char *rgbbuf2;
    rgbbuf2 = (unsigned char *)malloc(width*height*3);
    src_offset = 0;
    for(src_y=0;src_y<height;src_y++){
      dst_offset = height-src_y-1;
      for(src_x=0;src_x<width;src_x++) {
        src_p = pixels +3*src_offset;
        dst_p = rgbbuf2+3*dst_offset;
        dst_p[0] = src_p[0];
        dst_p[1] = src_p[1];
        dst_p[2] = src_p[2];
        src_offset++;
        dst_offset += height;
      }
    }
    int tmp;
    tmp = width;
    width=height;
    height=tmp;
    free(pixels);
    pixels=rgbbuf2;
  }


  void rotate270(void) {
    unsigned char *src_p, *dst_p;
    int src_x, src_y, src_offset, dst_offset;
    unsigned char *rgbbuf2;
    rgbbuf2 = (unsigned char *)malloc(width*height*3);
    src_offset = 0;
    for(src_y=0;src_y<height;src_y++){
      dst_offset = src_y+height*(width-1);
      for(src_x=0;src_x<width;src_x++) {
        src_p = pixels +3*src_offset;
        dst_p = rgbbuf2+3*dst_offset;
        dst_p[0] = src_p[0];
        dst_p[1] = src_p[1];
        dst_p[2] = src_p[2];
        src_offset++;
        dst_offset -= height;
      }
    }
    int tmp;
    tmp = width;
    width=height;
    height=tmp;
    free(pixels);
    pixels=rgbbuf2;
  }
};

#endif  // SCALED_IMAGE_H__

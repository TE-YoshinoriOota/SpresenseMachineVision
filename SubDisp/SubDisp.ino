/*
 *  SubDisp.ino - Display out for Spresense_mvision_camera
 *  Copyright 2024 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <MP.h>
#include <MPMutex.h>
MPMutex mutex(MP_MUTEX_ID0);

#include <Adafruit_ILI9341.h>
#define TFT_DC  9
#define TFT_CS  10
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);
extern const unsigned char stdfont[];


#define IMG_WIDTH  (320)
#define IMG_HEIGHT  (240)
#define FONT_WIDTH  (6)
#define THICKNESS (5)

struct det {
  bool exists;
  int16_t sx;
  int16_t sy;
  int16_t width;
  int16_t height;
  float x_mm;
  float y_mm;
};

struct region {
  struct det det[8];
  uint8_t *img;    
  float distance; 
  float h_fov;
  float v_fov;  
};

struct region *area;

uint16_t img[IMG_WIDTH*IMG_HEIGHT];
float h_tan_value = 0.0;
float v_tan_value = 0.0;


void draw_fillrect(uint16_t* buf, int16_t sx, int16_t sy, int16_t w, int16_t h, uint16_t color) {
  if (sx < 0) sx = 0;
  if (sy < 0) sy = 0;
  if (sx + w > IMG_WIDTH) w = IMG_WIDTH - sx;
  if (sy + h > IMG_HEIGHT) h = IMG_HEIGHT - sy;

  for (int y = sy; y < sy + h; ++y) {
    for (int x = sx; x < sx + w; ++x) {
      buf[y*IMG_WIDTH + x] = color;
    }
  }  
}

void draw_char(uint16_t* buf, int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {
  if ((x >= IMG_WIDTH)           || // Clip right
      (y >= IMG_HEIGHT)          || // Clip bottom
      ((x + 6*size_x - 1) < 0)   || // Clip left
      ((y + 8*size_y - 1) < 0))     // Clip top
      return;

  if (c >= 176) c++; // Handle 'classic' charset behavior

  for (int8_t i = 0; i < 5; ++i) { // Char bitmap = 5 columns
    uint8_t line = stdfont[c*5 + i];
    for (int8_t j = 0; j < 8; ++j, line >>= 1) {
      if (line & 1) {
        if (size_x == 1 && size_y == 1) {
          buf[(y+j)*IMG_WIDTH + (x+i)] = color;
        } else {
          draw_fillrect(buf, x+i*size_x, y+j*size_y, size_x, size_y, color);
        }
      } else if (bg != color) {
        if (size_x == 1 && size_y == 1) {
          buf[(y+j)*IMG_WIDTH + (x+i)] = bg;
        } else {
          draw_fillrect(buf, x+i*size_x, y+j*size_y, size_x, size_y, bg);
        }
      }
    }
  }

  if (bg != color) { // If opaque, draw vertical line for last column
    if (size_x == 1 && size_y == 1) {
      for (int i = 0; i < 8; ++i) {
        if ((x+5+i) >= IMG_WIDTH) break;
        buf[y*IMG_WIDTH + (x+5+i)] = bg;
      }
    } else {
      if ((x+5*size_x + size_x) < IMG_WIDTH) { 
        draw_fillrect(buf, x+5*size_x, y, size_x, 8*size_y, bg);
      }
    }
  }
}

void draw_box(uint16_t* buf, int sx, int sy, int w, int h, float x, float y) {
  const int thickness = 4; // BOXの線の太さ
  
  if (sx < 0 || sy < 0 || w < 0 || h < 0) { 
    Serial.println("draw_box parameter error");
    return;
  }

  if (sx+w >= IMG_WIDTH) 
    w = IMG_WIDTH-sx-1;
  if (sy+h >= IMG_HEIGHT) 
    h = IMG_HEIGHT-sy-1;
  
  /* draw the horizontal line of the square */
  for (int j = sx; j < sx+w; ++j) {
    for (int n = 0; n < thickness; ++n) {
      buf[(sy+n)*IMG_WIDTH + j] = ILI9341_BLUE;
      buf[(sy+h-n)*IMG_WIDTH + j] = ILI9341_BLUE;
    }
  }

  /* draw the vertical line of the square */
  for (int i = sy; i < sy+h; ++i) {
    for (int n = 0; n < thickness; ++n) { 
      buf[i*IMG_WIDTH+sx+n] = ILI9341_BLUE;
      buf[i*IMG_WIDTH+sx+w-n] = ILI9341_BLUE;
    }
  }

  /* draw the text area */
  const int box_height = 16;
  const int box_width = 30;
  int start_y = sy+h+thickness;
  int end_y = sy+h+thickness+box_height;
  if (start_y > IMG_HEIGHT-20 || end_y > IMG_HEIGHT-20) {
    return; /* out of range, no operation just return */
  }
  int start_x = sx+w/2-box_width;
  int end_x = sx+w/2+box_width;
  if (start_x < 0 || end_x > IMG_WIDTH) {
    return; /* out of range, no operation just return */
  }

  for (int i = start_y; i < end_y; ++i) {
    for (int j = start_x; j < end_x; ++j) {
      buf[i*IMG_WIDTH + j] = ILI9341_BLACK;
    }
  }
  
  /* draw coordinate info */
  const int font_size = 1;
  String str = String(x,0)+", "+String(y,0);
  int len = str.length();
  int cx = start_x+2;
  if (cx < 0) cx = 0;  
  for (int n = 0; n < len; ++n) {
    cx += FONT_WIDTH*font_size;
    if (cx >= IMG_WIDTH) break;
    char c = str.charAt(n);
    draw_char(buf, cx, start_y+4, c, ILI9341_YELLOW, ILI9341_BLACK, 1, 1);
  }
  return;
}

void draw_coordinate(uint16_t* buf, int thickness, float distance) {
  const int tick_size = 10;
 
  /* draw horizontal coordinate */
  for (int i = (IMG_HEIGHT-thickness)/2; i < (IMG_HEIGHT+thickness)/2; ++i) {
    for (int j = 0; j < IMG_WIDTH; ++j) {
      buf[i*IMG_WIDTH + j] = ILI9341_RED;
    }
  }

  float half_width = distance*h_tan_value;
  float half_width_pix_per_mm = (IMG_WIDTH/2)/half_width;
  for (int n = 10; n < 150; n += 10) {
    uint16_t mm10 = half_width_pix_per_mm*n;
    if (mm10 > IMG_WIDTH/2) break;
    for (int i = IMG_HEIGHT/2 - tick_size; i <= IMG_HEIGHT/2 + tick_size; ++i) {
      buf[i*IMG_WIDTH + IMG_WIDTH/2 + mm10] =  ILI9341_RED;
      buf[i*IMG_WIDTH + IMG_WIDTH/2 - mm10] =  ILI9341_RED;
    }
  }

  /* draw vertical coordinate */
  for (int j = (IMG_WIDTH-thickness)/2; j < (IMG_WIDTH+thickness)/2; ++j) {
    for (int i = 0; i < IMG_HEIGHT; ++i) {
      buf[i*IMG_WIDTH + j] = ILI9341_RED;
    }
  }

  float half_height = distance*v_tan_value;
  float half_height_pix_per_mm = (IMG_HEIGHT/2)/half_height;
  for (int n = 10; n < 100; n += 10) {
    uint16_t mm10 = half_height_pix_per_mm*n;
    if (mm10 > IMG_HEIGHT/2) break;
    for (int j = IMG_WIDTH/2 - tick_size; j <= IMG_WIDTH/2 + tick_size; ++j) {
      buf[(IMG_HEIGHT/2 + mm10)*IMG_WIDTH + j] =  ILI9341_RED;
      buf[(IMG_HEIGHT/2 - mm10)*IMG_WIDTH + j] =  ILI9341_RED;
    }
  }  
}

void draw_position(uint16_t *buf, int p_x, int p_y) {
  const int n = 2;
  for (int j = p_x-n; j < p_x+n; ++j) {
    buf[p_y*IMG_WIDTH + j] = ILI9341_WHITE;
  }
  for (int i = p_y-n; i < p_y+n; ++i) {
    buf[i*IMG_WIDTH + p_x] = ILI9341_WHITE;
  }
}

void draw_distance(uint16_t *buf, float distance) {
  for (int y = 220; y < IMG_HEIGHT; ++y) {
    for (int x = 0; x < IMG_WIDTH/2; ++x) {
      buf[y*IMG_WIDTH + x] = buf[y*IMG_WIDTH + IMG_WIDTH-1-x]= ILI9341_BLACK;
    }
  }

  const int font_size = 2;
  String str = "Distance = " + String(distance,1) + " mm";
  int len = str.length();
  int sx = 10;
  for (int n = 0; n < len; ++n) {
    sx += FONT_WIDTH*font_size;
    if (sx >= IMG_WIDTH) break;
    char c = str.charAt(n);
    draw_char(buf, sx, 225, c, ILI9341_YELLOW, ILI9341_BLACK, font_size, font_size);
  }
}

void setup() {
  display.begin();
  display.setRotation(3);

  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int ret;
  int8_t msgid;
  ret = MP.Recv(&msgid, &area);
  if (ret < 0) return;

  ret = mutex.Trylock();
  if (ret != 0) return;

  float hFoV = area->h_fov;
  float vFoV = area->v_fov;
  v_tan_value = tan(vFoV);
  h_tan_value = tan(hFoV); 

  // monochrome
  for (int y = 0; y < IMG_HEIGHT; ++y) {
    for (int x = 0; x < IMG_WIDTH; ++x) {
      uint16_t value = area->img[y*IMG_WIDTH + x];
      uint16_t red = (value & 0xf8) << 8;
      uint16_t green = (value & 0xfc) << 3;
      uint16_t blue = (value & 0xf8) >> 3;
      uint16_t pix = red | green | blue;
      img[y*IMG_WIDTH + x] = pix;
    }
  }

  // draw x-y coordinate
  draw_coordinate(&img[0], 3, area->distance);

  // draw box on the image
  for (int n = 0; n < 8; ++n) {
    if (area->det[n].exists) {
      draw_box(&img[0], area->det[n].sx, area->det[n].sy, area->det[n].width, area->det[n].height, area->det[n].x_mm, area->det[n].y_mm); 
      draw_position(&img[n], area->det[n].sx+area->det[n].width/2, area->det[n].sy+area->det[n].height/2); 
    }
  }

  // draw distance information on the image
  draw_distance(&img[0], area->distance);

  // transfer the graphic buffer to LCD
  display.drawRGBBitmap(0, 0, &img[0], IMG_WIDTH, IMG_HEIGHT);
  mutex.Unlock();
}
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

#define IMG_WIDTH  (320)
#define IMG_HEIGHT  (240)
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
};

struct region *area;

uint16_t img[IMG_WIDTH*IMG_HEIGHT];

float h_tan_value = 0.0;
float v_tan_value = 0.0;

bool draw_box(uint16_t* buf, int sx, int sy, int w, int h) {
  const int thickness = 4; // BOXの線の太さ
  
  if (sx < 0 || sy < 0 || w < 0 || h < 0) { 
    Serial.println("draw_box parameter error");
    return false;
  }
  if (sx+w >= IMG_WIDTH) 
    w = IMG_WIDTH-sx-1;
  if (sy+h >= IMG_HEIGHT) 
    h = IMG_HEIGHT-sy-1;
  
  /* draw the horizontal line of the square */
  for (int j = sx; j < sx+w; ++j) {
    for (int n = 0; n < thickness; ++n) {
      buf[(sy+n)*IMG_WIDTH + j] = ILI9341_RED;
      buf[(sy+h-n)*IMG_WIDTH + j] = ILI9341_RED;
    }
  }
  /* draw the vertical line of the square */
  for (int i = sy; i < sy+h; ++i) {
    for (int n = 0; n < thickness; ++n) { 
      buf[i*IMG_WIDTH+sx+n] = ILI9341_RED;
      buf[i*IMG_WIDTH+sx+w-n] = ILI9341_RED;
    }
  }
  return true;
}

void draw_coordinate(uint16_t* buf, int thickness, float distance) {
  /* draw horizontal coordinate */
  for (int i = (IMG_HEIGHT-thickness)/2; i < (IMG_HEIGHT+thickness)/2; ++i) {
    for (int j = 0; j < IMG_WIDTH; ++j) {
      buf[i*IMG_WIDTH + j] = ILI9341_RED;
    }
  }

  const int tick_size = 10;
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

void setup() {
  static const float vFoV = 2*M_PI*(31.2/2)/360.0;
  static const float hFoV = 2*M_PI*(41.8/2)/360.0;
  v_tan_value = tan(vFoV);
  h_tan_value = tan(hFoV);

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


  // 認識対象のボックスをカメラ画像に描画
  for (int n = 0; n < 8; ++n) {
    if (area->det[n].exists) {
      draw_box(&img[0], area->det[n].sx, area->det[n].sy, area->det[n].width, area->det[n].height); 
      draw_position(&img[n], area->det[n].sx+area->det[n].width/2, area->det[n].sy+area->det[n].height/2); 
    }
  }
  draw_coordinate(&img[0], 3, area->distance);
  
  // ボックス描画されたカメラ画像を表示
  display.drawRGBBitmap(0, 0, &img[0], IMG_WIDTH, IMG_HEIGHT);
  display.setCursor(0, 0);
  display.setTextColor(ILI9341_RED);
  display.setTextSize(2);  
  display.println("d = " + String(area->distance, 1) + " (" + String(area->det[0].x_mm, 1) + ", " + String(area->det[0].y_mm, 1) + ")");

  mutex.Unlock();
}
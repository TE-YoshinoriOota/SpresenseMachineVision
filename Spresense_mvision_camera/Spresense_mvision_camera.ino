/*
 *  semaseg_camera.ino - Binary Sematic Segmentation sample
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
#include <math.h>
#include <Camera.h>
#include <MP.h>
#include <MPMutex.h>
MPMutex mutex(MP_MUTEX_ID0);

const int dispcore = 1;
const int soniccore = 2;

#define IMG_WIDTH   (320)
#define IMG_HEIGHT  (240)
#define H_FOV  (41.8) /* spec typical value */
#define V_FOV  (31.2) /* spec typical value */

// #define PRINT_DEBUG 

uint32_t last_time = 0;
uint8_t disp[IMG_WIDTH*IMG_HEIGHT];
const float vFoV = 2*M_PI*(V_FOV/2)/360.0;
const float hFoV = 2*M_PI*(H_FOV/2)/360.0;
const float adjustment_mm = 10.5;

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

struct region area;

struct sonicdata {
  int duration;
  float distance;
};

struct sonicdata *dist;
#define DIST_DATA_SIZE 5
float dist_data[DIST_DATA_SIZE];

float h_tan_value = 0.0;
float v_tan_value = 0.0;

void CamCB(CamImage img) {
  int ret;
  uint8_t *buf; // buffer for the camera image

  if (!img.isAvailable()) return;

  uint32_t current_time = millis();
  uint32_t duration = current_time - last_time;
  last_time = current_time;
  Serial.println(String(duration) + " msec");

  // get the grayscale image
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_GRAY);
  buf = img.getImgBuff();

  memset(&area, 0, sizeof(struct region));
  bool result = detect_objects(buf, 0, 0, IMG_WIDTH, IMG_HEIGHT, &area, 0);

  int8_t sndid2 = 110;
  uint32_t dummy = 0;
  ret = MP.Send(sndid2, dummy, soniccore);
  if (ret >= 0) {
    int8_t msgid;
    MP.Recv(&msgid, &dist, soniccore);
  }
  static int p = 0;
  if (p >= DIST_DATA_SIZE) p = 0;
  dist_data[p++] = dist->distance;
  float ave_dist = 0.0;
  for (int n = 0; n < DIST_DATA_SIZE; ++n) {
    ave_dist += dist_data[n];
  } 
  ave_dist /= DIST_DATA_SIZE;
  ave_dist += adjustment_mm;

  float h_length_mm = ave_dist*h_tan_value;
  float v_length_mm = ave_dist*v_tan_value;
  float h_mm_per_pixel = h_length_mm / 160.0; // mm/pixel
  float v_mm_per_pixel = v_length_mm / 120.0; // mm/pixel

  if (result) {
    for (int n = 0; n < 8; ++n) {
      if (area.det[n].exists) {
        int sx = area.det[n].sx;
        int sy = area.det[n].sy;
        int width = area.det[n].width;
        int height = area.det[n].height;

      #ifdef PRINT_DEBUG
        Serial.println("area.det[" + String(n) + "].sx = " + String(sx));
        Serial.println("area.det[" + String(n) + "].sy = " + String(sy));
        Serial.println("area.det[" + String(n) + "].width = " + String(width));
        Serial.println("area.det[" + String(n) + "].height = " + String(height));
      #endif

        area.det[n].x_mm = ((sx + width/2) - IMG_WIDTH/2)*h_mm_per_pixel;
        area.det[n].y_mm = (IMG_HEIGHT/2 - (sy + height/2))*v_mm_per_pixel;
      }
    }
  }

  ret = mutex.Trylock();
  if (ret == 0) {
    memcpy(&disp[0], &buf[0], IMG_WIDTH*IMG_HEIGHT*sizeof(uint8_t));
    area.img = &disp[0];
    area.distance = ave_dist;
    area.h_fov = hFoV;
    area.v_fov = vFoV;
    int8_t sndid1 = 100;
    MP.Send(sndid1, &area, dispcore);
    mutex.Unlock();
  }
}


void setup() {
  Serial.begin(115200);

  v_tan_value = tan(vFoV);
  h_tan_value = tan(hFoV);

  MP.begin(dispcore);
  MP.begin(soniccore);
  MP.RecvTimeout(MP_RECV_POLLING);

  theCamera.begin();
  theCamera.startStreaming(true, CamCB);
}

void loop() {
  // put your main code here, to run repeatedly:

}

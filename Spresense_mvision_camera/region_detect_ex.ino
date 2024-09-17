/*
 *  region_detect.ino - Region detect sample for the binary semantic segmentation sample
 *  Copyright 2022 Sony Semiconductor Solutions Corporation
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


// Object recognition threshold value
const uint8_t threshold = 70;

// Window size for searching the area
const int window_h = 60;
const int window_v = 60;

// Minimum object size. 
// If the area is smaller than this value, the area is recognized as a noise.
const int object_min_size_h = 20;
const int object_min_size_v = 20;

/**
 * Function: get_region
 * Find the start position and the size of the object within the specified area
 * Paramters:
 *   offset_x: Horizontal offset value of the area to search
 *   offset_y: Vertical offset value of the area to search
 *   width : Width of the area to search
 *   height: Height of the area to search
 *   s_sx: X pixel of the start position at the left-top of the object 
 *   s_width: Width of the object. if there is no object, return zero.
 *   s_sy: Y pixel of the start position at the left-top of the object
 *   s_height: Width of the object. if there is no object, return zero.
 * Return:
 *   If the process is succeeded, return true
 *   If there are argument errors or calculation errors, return false.
 **/
bool get_region(uint8_t *img, 
  const int offset_x, const int offset_y, const int width, const int height, 
  int16_t* s_sx, int16_t* s_width, int16_t* s_sy, int16_t* s_height) 
{
  // Get the width and horizontal position of the object detected
  bool err;
  int16_t sx, swidth;
  err = get_sx_and_width_of_region(&img[0], offset_x, offset_y, width, height, &sx, &swidth);
  if (!err) {
    return false;
  }
  if (!err) {
    Serial.println("Error: detection error on x");
    return false;
  }
  if (sx >= IMG_WIDTH) {
    Serial.println("Error: detect x-poision error");
    return false;
  }
  if (swidth == 0) {
#ifdef PRINT_DEBUG
    Serial.println("Warning: no detection on x");
#endif
  }

  // Get the height and vertical position of the object detected
  int16_t sy, sheight;
  err = get_sy_and_height_of_region(&img[0], offset_x, offset_y, width, height, &sy, &sheight);
  if (!err) {
    Serial.println("Error: detection error on y");
    return false;
  }
  if (sy >= IMG_HEIGHT) {
    Serial.println("Error: detect y-poision error");
    return false;
  }
  if (sheight == 0) {
#ifdef PRINT_DEBUG
    Serial.println("Warning: no detection on y");
#endif
  }

  *s_sx = sx;
  *s_width = swidth;
  *s_sy = sy;
  *s_height = sheight;
  return true;
}


/** 
 * Function: region_inspector
 * to check whether the inspection area overlaps the detected area.
 * if the inspection area overlaps, return true and get the skip value of the x-coordinate.
 **/
bool region_inspector(int i, int j, struct region* area, int num, int* skip) {
  for (int n = 0; n < num; ++n) {
    int area_det_ex = area->det[n].sx + area->det[n].width;
    int area_det_ey = area->det[n].sy + area->det[n].height;
    if (i >= area->det[n].sy && i < area_det_ey && j >= area->det[n].sx && j < area_det_ex) {
      *skip = area_det_ex - j;
      return true;
    } 
  }
  return false;
}

/** 
 * Function: inspection_area_generator
 * to get the offset position and the size of the inspection winodw considering the detected areas.
 **/
void inspection_area_generator(int i, int j, struct region* area, int num, int* offset_x, int* offset_y, int* width, int* height) {
  // set the start point of vertical
  *offset_y = i;
  *height = window_v;
  if (*offset_y + *height > IMG_HEIGHT) *height = IMG_HEIGHT - *offset_y;

  // set the start point of horizontal 
  int win_length_right = window_h/2;
  int win_length_left = window_h/2;
  *offset_x = j - win_length_left;
  if (*offset_x < 0) { *offset_x = 0; win_length_left = j; }
  if (j + win_length_right > IMG_WIDTH) { win_length_right = IMG_WIDTH - j; }
  *width = win_length_left + win_length_right;

  // adjustment by area information not to overlap the horizontal area
  for (int n = 0; n < num; ++n) {
    int area_det_ex = area->det[n].sx + area->det[n].width;
    int area_det_ey = area->det[n].sy + area->det[n].height;

    // cehck left side overlap
    if (*offset_y >= area->det[n].sy && *offset_y < area_det_ey && *offset_x < area_det_ex && *offset_x >= area->det[n].sx) {
      int end_width = *offset_x + *width;
      *offset_x = area_det_ex;
      *width = end_width - *offset_x;
      if (*width < 0) *width = 0;
      return;
    }

    // check right side overlap
    if (*offset_y >= area->det[n].sy && *offset_y < area_det_ey && (*offset_x + *width) >= area->det[n].sx && (*offset_x + *width) < area_det_ex) {
      *offset_x = area->det[n].sx - *width;
      if (*offset_x < 0) *offset_x = 0;
      if (*width > area->det[n].sx) *width = area->det[n].sx;
      return;
    } 

    // check top side overlap
    if (*offset_y >= area->det[n].sy && *offset_y < area_det_ey && j >= area->det[n].sx && j < area_det_ex) {
      *offset_y = area_det_ey;
      if (*offset_y + *height > IMG_HEIGHT) *height = IMG_HEIGHT - *offset_y;
      return;
    }

    // check bottom side overlap
    if (*offset_y + *height >= area->det[n].sy && *offset_y + *height < area_det_ey && j >= area->det[n].sx && j < area_det_ex) {
      *offset_y = area->det[n].sy - *width;
      if (*offset_y < 0) *offset_y = 0;
      if (*height > area->det[n].sy) *height = area->det[n].sy;
      return;
    }

  }
}

/**
 * Function: detect_objects
 * Find the objects in the image data
 * Paramters:
 *   img: The grayscale image to analyze
 *   offset_x: Horizontal offset value of the image area to search
 *   offset_y: Vertical offset value of the image area to search
 *   win_width : Width of the image area to search
 *   wun_height: Height of the image area to search
 *   struct region* area: The structure to store the information of the objects detected
 *   num : The number of the objects detected.
 * Return:
 *   If the process is succeeded, return true
 *   If there are argument errors or calculation errors, return false.
 **/
bool detect_objects(uint8_t* img, const int offset_x, const int offset_y, const int win_width, const int win_height, struct region* area, int num) {

#ifdef PRINT_DEBUG
  Serial.println(String(__FUNCTION__) + "[" + String(num) + "] : Start");
#endif

  if (num >= 8) return true;

  // 引数チェック
  if (img == NULL || area == NULL || win_width > IMG_WIDTH || win_height > IMG_HEIGHT) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }

  for (int i = offset_y; i < win_height; ++i) {
    for (int j = offset_x; j < win_width; ++j)  {

      if (img[i*IMG_WIDTH + j] < threshold) { // find black point
        
        // check if the region is already inspected
        int skip = 0;
        bool result = region_inspector(i, j, area, num, &skip);
        if (result) { j += skip; continue; }

        int win_offset_x, win_offset_y;
        int win_width, win_height;
        inspection_area_generator(i, j, area, num, &win_offset_x, &win_offset_y, &win_width, &win_height);

        int16_t sx, sy, width, height;
        result = get_region(img, win_offset_x, win_offset_y, win_width, win_height, &sx, &width, &sy, &height);  
        if (result) {
          if (width > object_min_size_h && height > object_min_size_v) {
            area->det[num].exists = true;
            area->det[num].sx = sx;
            area->det[num].width = width;
            area->det[num].sy = sy;
            area->det[num].height = height;
            /*
            Serial.println("area.det[" + String(num) + "].sx = " + String(sx));
            Serial.println("area.det[" + String(num) + "].sy = " + String(sy));
            Serial.println("area.det[" + String(num) + "].width = " + String(width));
            Serial.println("area.det[" + String(num) + "].height = " + String(height));  
            */  
            ++num;
            return detect_objects(img, 0, sy, IMG_WIDTH, IMG_HEIGHT-sy, area, num);
          } else {
            // skip noise area
            j += width;  i += height;
          }
        }
      }
    }
  }

#ifdef PRINT_DEBUG
  Serial.println(String(__FUNCTION__) + "[" + String(num) + "] : FINISHED");
#endif

  return true;
}

/** 
 * Find the starting x position and width of the maximum horizontal area
 * Paramters:
 *   img: The grayscale image to analyze
 *   offset_x: Horizontal offset value of the image area to search
 *   offset_y: Vertical offset value of the image area to search
 *   width : Width of the image area to search
 *   height: Height of the image area to search
 *   s_sx: X pixel of the start position at the left-top of the object 
 *   s_width: Width of the object. if there is no object, return zero.
 *   s_sy: Y pixel of the start position at the left-top of the object
 *   s_height: Width of the object. if there is no object, return zero.
 * Return:
 *   If the process is succeeded, return true
 *   If there are argument errors or calculation errors, return false.
 **/
bool get_sx_and_width_of_region(uint8_t *img, 
  const int offset_x, const int offset_y, const int width, const int height, 
  int16_t* s_sx, int16_t* s_width) {
  
  // arguments check
  if (img == NULL || offset_x < 0 || offset_y < 0 
  || (offset_x + width) > IMG_WIDTH || (offset_y + height) > IMG_HEIGHT) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  
  // Find the maximum value of the map to get the object width and the end position
  int16_t max_h_val = -1;  // the maximum value of the horizontal consecutive line.
  int16_t max_h_point = -1; // the end position of the horizontal consecutive line.
  for (int i = offset_y; i < offset_y + height; ++i) {
    int h_val = 0;
    for (int j = offset_x; j < offset_x + width; ++j) {
      // if the pixel value is under the threshold, increase the value.
      if (img[i*IMG_WIDTH + j] < threshold) ++h_val;
      else h_val = 0; // if the pixel value is above the threshold, reset the value
      if (h_val > max_h_val) {
        max_h_val = h_val-1;
        max_h_point = j;
      }
    }
  }

  *s_sx = max_h_point - max_h_val; // the start x-position of the horizontal consecutive line
  *s_width = max_h_val; // the length of the horizontal consecutive line
  if (*s_sx < 0) return false;
  return true;
}


/** 
 * Find the starting y position and width of the maximum horizontal area
 * Paramters:
 *   img: The grayscale image to analyze
 *   offset_x: Horizontal offset value of the image area to search
 *   offset_y: Vertical offset value of the image area to search
 *   width : Width of the image area to search
 *   height: Height of the image area to search
 *   s_sy: Y pixel of the start position at the left-top of the object
 *   s_height: Width of the object. if there is no object, return zero.
 * Return:
 *   If the process is succeeded, return true
 *   If there are argument errors or calculation errors, return false.
 **/
bool get_sy_and_height_of_region(uint8_t *img,
  const int offset_x, const int offset_y, const int width, const int height,
  int16_t* s_sy, int16_t* s_height) 
{
  
  // arguments check
  if (img == NULL || offset_x < 0 || offset_y < 0 
  || (offset_x + width) > IMG_WIDTH || (offset_y + height) > IMG_HEIGHT) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  
  // Find the maximum value of the map to get the object width and the end position
  int max_v_val = -1;  // the maximum value of the vertical consecutive line.
  int max_v_point = -1;  // the end position of the vertical consecutive line.
  for (int j = offset_x; j < offset_x + width; ++j) {
    int v_val = 0;
    for (int i = offset_y; i < offset_y + height; ++i) {
      // if the pixel value is under the threshold, increase the value.    
      if (img[i*IMG_WIDTH + j] < threshold) ++v_val;
      else v_val = 0; // if the pixel value is above the threshold, reset the value
      if (v_val > max_v_val) {
        max_v_val = v_val-1;
        max_v_point = i;
      }
    }
  }
  
  *s_sy = max_v_point - max_v_val; // the start y-position of the vertical consecutive line
  *s_height = max_v_val; // the length of the vertical consecutive line
  if (*s_sy < 0) return false;
  return true;
}

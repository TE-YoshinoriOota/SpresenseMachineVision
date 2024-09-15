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


// ピクセルの連続値を記録する閾値
const uint8_t threshold = 60;

// 指定領域内の最大領域の開始座標(x,y)と幅と高さ(width, height)を検出する
//   offset_x: 入力画像をサーチする座標の横オフセット値
//   offset_y: 入力画像をサーチする座標の縦オフセット値
//   width : 入力画像をサーチする座標の幅
//   height: 入力画像をサーチする座標の高さ
//   s_sx: 最大領域の開始座標を得る
//   s_width: 最大領域の幅を得る　検出されなかった場合はゼロ
//   s_sy: 最大領域の開始座標を得る
//   s_height: 最大領域の高さを得る　検出されなかった場合はゼロ
//   返値：引数エラーもしくは演算エラーもしくは検出できなかった
bool get_region(uint8_t *img, 
  const int offset_x, const int offset_y, const int tgt_width, const int tgt_height, 
  int16_t* s_sx, int16_t* s_width, int16_t* s_sy, int16_t* s_height) {

  // 認識対象の横幅と横方向座標を取得
  bool err;
  int16_t sx, swidth;
  err = get_sx_and_width_of_region(&img[0], offset_x, offset_y, tgt_width, tgt_height, &sx, &swidth);
  if (!err) {
    return false;
  }
  if (!err) {
    Serial.println("detection error");
    return false;
  }
  if (sx >= IMG_WIDTH) {
    Serial.println("detect x-poision error");
    return false;
  }
  if (swidth == 0) {
    Serial.println("no detection");
    return false;
  }

  // 認識対象の縦幅と縦方向座標を取得
  int16_t sy, sheight;
  err = get_sy_and_height_of_region(&img[0], offset_x, offset_y, tgt_width, tgt_height, &sy, &sheight);
  if (!err) {
    Serial.println("detection error");
    return false;
  }
  if (sy >= IMG_HEIGHT) {
    Serial.println("detect y-poision error");
    return false;
  }
  if (sheight == 0) {
    Serial.println("no detection");
    return false;
  }

  *s_sx = sx;
  *s_width = swidth;
  *s_sy = sy;
  *s_height = sheight;
  return true;
}


// ウィンドウを分割して各領域の検出結果を記録

bool detect_objects(uint8_t* img, const int img_width, const int img_height, struct region* area) {

  // 引数チェック
  if (img == NULL || area == NULL || img_width < IMG_WIDTH || img_height < IMG_HEIGHT) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }

  bool result;
  int16_t sx, sy, width, height;
  // window0 (offset_x = 0, offset_y = 0, width = IMG_WIDTH/2, height = IMG_HEIGHT/2)
  result = get_region(img, 0, 0, img_width/2, img_height/2, &sx, &width, &sy, &height);
  if (result) {
    area->det[0].exists = true;
    area->det[0].sx = sx;
    area->det[0].sy = sy;
    area->det[0].width = width;
    area->det[0].height = height;
  }

  // window1 (offset_x = IMG_WIDTH/2, offset_y = 0, width = IMG_WIDTH/2, height = IMG_HEIGHT/2)
  result = get_region(img, img_width/2, 0, img_width/2, img_height/2, &sx, &width, &sy, &height);
  if (result) {
    area->det[1].exists = true;
    area->det[1].sx = sx;
    area->det[1].sy = sy;
    area->det[1].width = width;
    area->det[1].height = height;
  }
  
  // window2 (offset_x = IMG_WIDTH/2, offset_y = IMG_HEIGHT/2, width = IMG_WIDTH/2, height = IMG_HEIGHT/2)
  result = get_region(img, img_width/2, img_height/2, img_width/2, img_height/2, &sx, &width, &sy, &height);
  if (result) {
    area->det[2].exists = true;
    area->det[2].sx = sx;
    area->det[2].sy = sy;
    area->det[2].width = width;
    area->det[2].height = height;
  }

  // window3 (offset_x = 0, offset_y = IMG_HEIGHT/2, width = IMG_WIDTH/2, height = IMG_HEIGHT/2)
  result = get_region(img, 0, img_height/2, img_width/2, img_height/2, &sx, &width, &sy, &height);
  if (result) {
    area->det[3].exists = true;
    area->det[3].sx = sx;
    area->det[3].sy = sy;
    area->det[3].width = width;
    area->det[3].height = height;
  }

  // marge process
  int sx0 = area->det[0].sx;
  int ex0 = area->det[0].width + sx0;
  int sy0 = area->det[0].sy;
  int ey0 = area->det[0].height + sy0;

  int sx1 = area->det[1].sx;
  int ex1 = area->det[1].width + sx1;
  int sy1 = area->det[1].sy;
  int ey1 = area->det[1].height + sy1;

  int sx2 = area->det[2].sx;
  int ex2 = area->det[2].width + sx2;
  int sy2 = area->det[2].sy;
  int ey2 = area->det[2].height + sy2;

  int sx3 = area->det[3].sx;
  int ex3 = area->det[3].width + sx3;
  int sy3 = area->det[3].sy;
  int ey3 = area->det[3].height + sy3;

  // check whether window0 and window1 
  if (ex0 == img_width/2-1 && sx1 == img_width/2-1) {
    area->det[0].width += area->det[1].width;
    area->det[1].exists = false;
  }

  // check whether window1 and window2
  if (ey1 == img_height/2-1 && sy2 == img_height/2-1) {
    area->det[1].height += area->det[2].height;
    area->det[2].exists = false;
  }

  // check whether window2 and window3
  if (sx2 == img_width/2-1 && ex3 == img_width/2-1) {
    area->det[3].width += area->det[2].width;
    area->det[2].exists = false;
  }
  
  // check whether window3 and window0
  if (sy3 == img_height/2-1 && ey0 == img_height/2-1) {
    area->det[0].height += area->det[3].height;
    area->det[3].exists = false;
  }

  return result;
}


// 横方向の最大領域の開始座標(x)と幅を検出する
//   offset_x: 入力画像をサーチする座標の横オフセット値
//   offset_y: 入力画像をサーチする座標の縦オフセット値
//   width : 入力画像をサーチする座標の幅
//   height: 入力画像をサーチする座標の高さ
//   s_sx: 最大領域の開始座標を得る
//   s_width: 最大領域の幅を得る　検出されなかった場合はゼロ
//   返値：引数エラーもしくは演算エラー
bool get_sx_and_width_of_region(uint8_t *img, 
  const int offset_x, const int offset_y, const int width, const int height, 
  int16_t* s_sx, int16_t* s_width) {
  
  // 引数チェック
  if (img == NULL || offset_x < 0 || offset_y < 0 
  || (offset_x + width) > IMG_WIDTH || (offset_y + height) > IMG_HEIGHT) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  
  // マップの最大値を探し、最大領域の横幅と終端座標を得る
  int16_t max_h_val = -1;  // 最大幅（水平方向）を格納
  int16_t max_h_point = -1; // 最大幅の終了座標
  for (int i = offset_y; i < offset_y + height; ++i) {
    int h_val = 0;
    for (int j = offset_x; j < offset_x + width; ++j) {
      // ピクセルの出力が閾値より下の場合は加算
      if (img[i*IMG_WIDTH + j] < threshold) ++h_val;
      else h_val = 0; // 閾値以上は0リセット
      if (h_val > max_h_val) {
        max_h_val = h_val;
        max_h_point = j;
      }
    }
  }

  *s_sx = max_h_point - max_h_val; // 開始座標(x)
  *s_width = max_h_val; // 最大領域の幅
  if (*s_sx < 0) return false;
  return true;
}


// 縦方向の最大領域の開始座標(y)と高さを検出する
//   offset_x: 入力画像をサーチする座標の横オフセット値
//   offset_y: 入力画像をサーチする座標の縦オフセット値
//   width : 入力画像をサーチする座標の幅
//   height: 入力画像をサーチする座標の高さ
//   s_sy: 最大領域の開始座標を得る
//   s_height: 最大領域の高さを得る　検出されなかった場合はゼロ
//   返値：引数エラーもしくは演算エラー
bool get_sy_and_height_of_region(uint8_t *img,
  const int offset_x, const int offset_y, const int width, const int height,
  uint16_t* s_sy, uint16_t* s_height) {
  
  // 引数チェック
  if (img == NULL || offset_x < 0 || offset_y < 0 
  || (offset_x + width) > IMG_WIDTH || (offset_y + height) > IMG_HEIGHT) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  
  // マップの最大値を探し、最大領域の縦幅と終端座標を得る
  int max_v_val = -1;  // 最大幅（高さ方向）を格納
  int max_v_point = -1;  // 最大高さの終了座標
  for (int j = offset_x; j < offset_x + width; ++j) {
    int v_val = 0;
    for (int i = offset_y; i < offset_y + height; ++i) {
      // ピクセルの出力が閾値より下の場合は加算     
      if (img[i*IMG_WIDTH + j] < threshold) ++v_val;
      else v_val = 0;  // 閾値以上は0リセット
      if (v_val > max_v_val) {
        max_v_val = v_val;
        max_v_point = i;
      }
    }
  }
  
  *s_sy = max_v_point - max_v_val; // 開始座標(y)
  *s_height = max_v_val; // 最大領域の高さ
  if (*s_sy < 0) return false;
  return true;
}

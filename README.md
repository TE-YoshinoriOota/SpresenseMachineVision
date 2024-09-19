# SpresenseMachineVision
Machine Vision Sample using Spresense HDR Camera

Spresense Machine Vision can recognize objects from grayscale images.
It is a straightforward method of image processing that detects dark areas. This example can recognize the positions of multiple objects by a distance sensor using a supersonic.

![image](https://github.com/user-attachments/assets/27331aab-aca9-4065-966f-0aa7f871d9a6)

## Theory of the Machine Vision
The key to understanding the measurement by a captured image is the knowledge of camera systems. Once you get a captured image by a camera, you cannot know the real length of objects only by the image itself. Converting a captured object's pixel length to the real world's length requires the lens characteristics of FOV (Field of View).

Spresense HDR Camera's Horizontal/Vertical Field of View are as follows.

<br/>


| Spresense HDR Camera     | degree |
|--------------------------|--------|
| Horizontal Field of View |  41.8° |
| Vertical Filed of View   |  31.2° |

<br/>

Once you can get a distance from the camera, you can get the length (mm) per pixel by FoV information and the resolution of the image as follows.

<br/><br/>

![image](https://github.com/user-attachments/assets/ff31c43e-e5e7-4920-ad03-f9140a6fb183)

<br/>

## Algorithm of the region detection
The region detection algorithm is a straightforward method that is simple but fast. The algorithm has 3 steps.

(1) Find a black point under the threshold
![image](https://github.com/user-attachments/assets/1da91197-13e4-44c4-929a-e7bf728402b0)


(2) Scan horizontally and vertically from the starting point calculated by the discovered black point and the window_h specified and search for the maximum count of consecutive black points.
![image](https://github.com/user-attachments/assets/40ba37b6-6a63-4489-9326-6d8e220901d5)


(3) To find the next region correctly, record the found regions and skip the area for the consequent process. 
![image](https://github.com/user-attachments/assets/d51d5639-94cb-40ba-8cda-fb9b1a045f3f)


By Spresense, this process takes under 104 milliseconds.

## System configuration
The system is straightforward. Here are the parts I used.

|Parts                        | Memo |
|-----------------------------|--------|
| Spresense Main Board        |   |
| Spresense Extension Board   |   |
| Spresense HDR Camera Board   |   |
| HC-SR04                      | Ultrasonic distance sensor |
| ILI9341 2.2inch           | TFT Display |


![image](https://github.com/user-attachments/assets/4d0bd6dc-cb80-4644-9acd-726761be63fc)


Making a measurement software using HC-SR04 requires considering the gap between the ultrasonic sensor and the image sensor.
In my case, the gap is almost 10mm by adjusting it through the camera.

![image](https://github.com/user-attachments/assets/f34348db-cf89-4047-a37a-ac99f35c0d29)



## Software configuration
The role of each core is as follows.

![image](https://github.com/user-attachments/assets/b0459f23-e3f9-4796-9b0c-f1e38ba16589)



### maincore
The maincore's major role is to capture an image and detect regions under the specified threshold.
The maincore program is in the "Spresense_mvision_camera" folder.

| file name | function  |
|-----------|-----------|
| Spresense_mvision_camera.ino | (1) capturing images <br/> (2) control subcores <br/> (3) converting a pixel length to the physical length |
| region_detct_ex.ino      | (1) detecting regions under the specified threshold in the captured image |

The threshold is defined at the code below in "Spresense_mvision_camera.ino"

```
// Object recognition threshold value
const uint8_t threshold = 70;
```

Detection areas are stored in the structure below

```
struct det {
  bool exists;  // detection area is valid or not
  int16_t sx;   // the offset in x coordinate of this area
  int16_t sy;   // the offset in y coordinate of this area
  int16_t width;  // the width of this area
  int16_t height; // the height of this area
  float x_mm;  // the center position of this area from the center of the image
  float y_mm;  // the center position of this area from the center of the image
};

struct region {
  struct det det[AREA_MAX_NUM];  // array of the detected area
  uint8_t *img;  // captured image in grayscale
  float distance;  // the distance between the objects and the camera.
  float h_fov;  // horizontal field of view of the camera
  float v_fov;  // vertical field of view of the camera
};
```

The region detection API is "detect_objects". It is called in "Spresense_mvsion_camera.ino" as follows.

```
  // get the grayscale image
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_GRAY);
  buf = img.getImgBuff();

  memset(&area, 0, sizeof(struct region));
  bool result = detect_objects(buf, threshold, 0, 0, IMG_WIDTH, IMG_HEIGHT, &area, 0);
```

| arguments of detect_objects | description |
|-----------------------------|-------------|
|  uint8_t* img | The grayscale image to analyze |
|  const uint8_t threshold | threshold pixel value for region detection |
|  const int offset_x | Horizontal offset value of the image area to search |
|  const int offset_y | Vertical offset value of the image area to search |
|  const int win_width | Width of the image area to search |
|  const int win_heightt | Height of the image area to search |
|  struct region* area | The structure to store the information of the objects detected |
|  int num | The number of the objects detected |

To measure the physical position of the objects, you need to get the distance between the plane of the objects and the camera.
The distance measured by the subcore2 can be acquired by the code below in "Spresense_mvision_camera.ino. 
To get less noise distance data, this code makes an average operation by storing past distance data in the dist_data array.

```
int8_t sndid2 = 110;
  uint32_t dummy = 0;
  ret = MP.Send(sndid2, dummy, soniccore);
  if (ret >= 0) {
    int8_t msgid;
    MP.Recv(&msgid, &dist, soniccore);
  }
  static int p = 0;
  if (p >= DIST_AVERAGE_SIZE) p = 0;
  dist_data[p++] = dist->distance;
  float ave_dist = 0.0;
  for (int n = 0; n < DIST_AVERAGE_SIZE; ++n) {
    ave_dist += dist_data[n];
  } 
  ave_dist /= DIST_AVERAGE_SIZE;
  ave_dist += adjustment_mm;
```

To display the result on the LCD, the image and the detected object information stored in the struct region passes the subcore-1.
Please note that you need to copy the image to another memory area because the current image buffer will be reused to capture another scene.
Since the subcore1 runs concurrently if you pass the original image buffer to the subcore1, the image will be corrupted when the capture process starts.

```
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
```


### subcore1 (Display)
The subcore1 draws current information on the LCD

(1) Captured image
(2) XY coordinates with markings every 1 cm
(3) Boxes of the detected objects
(4) Center positions of each object
(5) Distance between the camera and the object in millimeters

The object detection data is received by the subcore1 as follows

```
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

  // transfer the graphic buffer to the LCD display.
  display.drawRGBBitmap(0, 0, &img[0], IMG_WIDTH, IMG_HEIGHT);
  mutex.Unlock();
```

### subcore2 (Ultrasonic)
The subcore2 measures the distance between the camera and the object.
Since the distance measured with the ultrasonic sensor is very noisy, the observed data is applied by a simple Kalman filter based on the local level model which is a state-space model assuming that the object is not moving. You need to adjust the process noise variance and the observation noise variance according to your distance sensor, if needed. In my case, I set the process noise variance was 0.1 and the observation noise variance was 100. The distance data is transferred by request from the maincore like the client-server model.

```
void loop() {
  delay(10); // wait 10 milli seconds
  digitalWrite(Trig, LOW); // Trigger Low for ready
  delayMicroseconds(1); // wait for 1 micro second
  digitalWrite(Trig, HIGH); // Trigger High for output 
  delayMicroseconds(11); // Signal output for 10 micro seconds
  digitalWrite(Trig, LOW); // Stop the signal
  Duration = pulseIn(Echo, HIGH); // measure the time for the reflected signal
  if (Duration > 0) {
    // Calculate the distance by the time of the reflected signal
    Distance = Duration/2;
    Distance = Distance*340*1000/1000000;
  }

  kalman_update(&kf, Distance);

  int ret;
  int8_t msgid;
  uint32_t dummy;
  ret = MP.Recv(&msgid, &dummy);
  if (ret < 0) return;

  int8_t sndid = 110;
  data.duration = Duration;
  data.distance = kf.x;
  MP.Send(sndid, &data);
  //printf("%f\n", Distance);
}
```


## Demonstration
coming soon...




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
The system is very simple. Here is the parts I used.

|Parts                        | Memo |
|-----------------------------|--------|
| Spresense Main Board        |   |
| Spresense Extension Board   |   |
| Spresense HDR Camera Board   |   |
| HC-SR04                      | Ultrasonic distance sensor |
| ILI9341 2.2inch           | TFT Display |


![image](https://github.com/user-attachments/assets/4d0bd6dc-cb80-4644-9acd-726761be63fc)


## Software configuration
### maincore
coming soon...

### subcore1 (Display)
coming soon...

### subcore2 (Ultrasonic)
coming soon...

## Demonstration
coming soon...




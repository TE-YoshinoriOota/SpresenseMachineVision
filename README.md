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

## Algorithm of the object detection

coming soon...

## System configuration

coming soon...

## Software configuration
### maincore
coming soon...

### subcore1 (Display)
coming soon...

### subcore2 (Ultrasonic)
coming soon...

## Demonstration
coming soon...




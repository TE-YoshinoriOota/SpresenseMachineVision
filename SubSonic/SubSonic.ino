#ifndef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <MP.h>

#define Trig 2
#define Echo 3
int Duration;
float Distance;

struct sonicdata {
  int duration;
  float distance;
};

// Structure for the Kalman filter
typedef struct {
  float x;    // State parameter (level)
  float P;    // Error covariance
  float Q;    // Process noise variance
  float R;    // Observation noise variance
} KalmanFilter;

struct sonicdata data;
KalmanFilter kf;

void setup() {
  // Initialize the Kalman filter
  float init_x = 0.0;     // initial state
  float init_P = 1.0;     // initial error covariance
  float process_noise = 0.1; // proccess noise variance
  float measurement_noise = 100.0; // observation noise variance

  // Initialize the kalman filter
  kalman_init(&kf, init_x, init_P, process_noise, measurement_noise);

  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT_PULLUP);
  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);
}

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
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

struct sonicdata data;

void setup() {
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT_PULLUP);
  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  delay(10); // 10ミリ秒まつ
  digitalWrite(Trig, LOW); //まずLowにする
  delayMicroseconds(1); //1μ秒まつ
  digitalWrite(Trig, HIGH); //トリガー信号をあげる
  delayMicroseconds(11); //10μ秒だが念のため11μ秒HIGHを出す
  digitalWrite(Trig, LOW); //信号を落とす
  Duration = pulseIn(Echo, HIGH);//パルスがHIGHの間の時間を測る
  if (Duration > 0) {
    //パルスの秒から距離を計算する
    Distance = Duration/2;
    Distance = Distance*340*1000/1000000;
  }


  int ret;
  int8_t msgid;
  uint32_t dummy;
  ret = MP.Recv(&msgid, &dummy);
  if (ret < 0) return;

  int8_t sndid = 110;
  data.duration = Duration;
  data.distance = Distance;
  MP.Send(sndid, &data);
  //printf("%f\n", Distance);
}
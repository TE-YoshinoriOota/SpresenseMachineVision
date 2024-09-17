/*
 *  Simple Kalman filter based on local level model
 *  Copyright (c) 2024 Yoshinori Oota
 */


#include <stdio.h>

// initialize the kalman filter
void kalman_init(KalmanFilter* kf, double init_x, double init_P, double Q, double R) {
    kf->x = init_x; // set the initial state
    kf->P = init_P; // set the initial error covariance
    kf->Q = Q; // set the process noise
    kf->R = R; // set the observation noise
}

// Update parameters of the kalman filter by observation data (z)
void kalman_update(KalmanFilter* kf, float z) {
    // Predistion step
    float P_pred = kf->P + kf->Q; // Update error covariance
    // Calculate kalman gain
    float K = P_pred / (P_pred + kf->R);

    // Update the state
    kf->x = kf->x + K * (z - kf->x);

    // Update error covariance
    kf->P = (1 - K) * P_pred;
}

/*
int main() {
    // カルマンフィルタの初期値を設定
    KalmanFilter kf;
    double init_x = 0.0;     // 初期状態
    double init_P = 1.0;     // 初期誤差共分散
    double process_noise = 1.0; // プロセスノイズの分散
    double measurement_noise = 2.0; // 観測ノイズの分散

    // カルマンフィルタの初期化
    kalman_init(&kf, init_x, init_P, process_noise, measurement_noise);

    // センサデータのシミュレーション
    double measurements[5] = {1.0, 2.0, 3.0, 2.5, 3.5};

    // 各観測値に対してカルマンフィルタを適用
    for (int i = 0; i < 5; i++) {
        kalman_update(&kf, measurements[i]);
        printf("Time %d: Estimated state x = %f\n", i, kf.x);
    }

    return 0;
}
*/
#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Kalman Filter 1D State Structure
 */
typedef struct {
    float q; // Process noise covariance
    float r; // Measurement noise covariance
    float x; // Value
    float p; // Estimation error covariance
    float k; // Kalman gain
} Kalman1D_t;

/**
 * @brief Initialize a 1D Kalman Filter
 * @param filter Pointer to the filter structure
 * @param q Process noise covariance (higher = trust measurement more, fast reaction)
 * @param r Measurement noise covariance (higher = trust prediction more, smooth output)
 * @param p Initial estimation error covariance
 * @param initial_value Initial value
 */
void Kalman_Init(Kalman1D_t *filter, float q, float r, float p, float initial_value);

/**
 * @brief Update the Kalman Filter with a new measurement
 * @param filter Pointer to the filter structure
 * @param measurement The new measurement value
 * @return The filtered/smoothed value
 */
float Kalman_Update(Kalman1D_t *filter, float measurement);

#endif // KALMAN_FILTER_H

#include "kalman_filter.h"

void Kalman_Init(Kalman1D_t *filter, float q, float r, float p, float initial_value)
{
    filter->q = q;
    filter->r = r;
    filter->p = p;
    filter->x = initial_value;
}

float Kalman_Update(Kalman1D_t *filter, float measurement)
{
    // Predict
    filter->p = filter->p + filter->q;

    // Update
    filter->k = filter->p / (filter->p + filter->r);
    filter->x = filter->x + filter->k * (measurement - filter->x);
    filter->p = (1 - filter->k) * filter->p;

    return filter->x;
}

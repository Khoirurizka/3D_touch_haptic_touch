#ifndef HAPTIC_LIB_H
#define HAPTIC_LIB_H

#include <HD/hd.h>
#include <HDU/hduVector.h>
#include <array>

struct Sample {
    std::array<double, 3> pos;    // mm
    std::array<double, 3> force;  // N (device-frame)
    int buttons;
    int errorCode;
    int internalErrorCode;
};

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the haptic device
int haptic_init();

// Start the servo loop
int haptic_start();

// Get the latest sample (position, force, buttons)
void haptic_get_sample(struct Sample* sample);

// Get the sample counter
unsigned long long haptic_get_sample_count();

// Set force feedback (e.g., from robot arm sensor)
void haptic_set_force(double force[3]);

// Stop and cleanup
void haptic_stop();

#ifdef __cplusplus
}
#endif

#endif // HAPTIC_LIB_H
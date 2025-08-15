#include "haptic_lib.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>
#include <algorithm>

static std::atomic<bool> running{true};
static Sample g_lastSample;
static std::atomic<uint64_t> g_sampleCounter{0};
static HHD g_hHD = HD_INVALID_HANDLE;
static HDSchedulerHandle g_hCallback = 0;
static std::atomic<int> warmMotorCount{0}; // New: Track consecutive HD_WARM_MOTORS errors

HDCallbackCode HDCALLBACK servoCallback(void* /*pUserData*/) {
    hdBeginFrame(hdGetCurrentDevice());
    hduVector3Dd pos, force;
    HDint buttons = 0;
    hdGetDoublev(HD_CURRENT_POSITION, pos);
    hdGetDoublev(HD_CURRENT_FORCE, force);
    hdGetIntegerv(HD_CURRENT_BUTTONS, &buttons);

    static HDint lastButtons = -1;
    if (buttons != lastButtons) {
        std::fprintf(stderr, "[DEBUG] Servo callback - Raw buttons: 0x%X\n", buttons);
        lastButtons = buttons;
    }

    g_lastSample.pos = {pos[0], pos[1], pos[2]};
    g_lastSample.force = {force[0], force[1], force[2]};
    g_lastSample.buttons = buttons;

    HDErrorInfo err = hdGetError();
    g_lastSample.errorCode = err.errorCode;
    g_lastSample.internalErrorCode = err.internalErrorCode;

    if (HD_DEVICE_ERROR(err)) {
        // Only log HD_WARM_MOTORS every 100 occurrences to reduce clutter
        static int logCounter = 0;
        // if (err.errorCode == 1024 && (logCounter++ % 100 == 0)) { // HD_WARM_MOTORS
        //     std::fprintf(stderr, "[DEBUG] Servo callback warning: Code=%d, Internal=%d, Force=[%f, %f, %f], WarmCount=%d\n",
        //                  err.errorCode, err.internalErrorCode, force[0], force[1], force[2], warmMotorCount.load());
        // } else if (err.errorCode != 1024) {
        //     std::fprintf(stderr, "[DEBUG] Servo callback error: Code=%d, Internal=%d, Force=[%f, %f, %f]\n",
        //                  err.errorCode, err.internalErrorCode, force[0], force[1], force[2]);
        // }

        if (err.errorCode == 1024) { // HD_WARM_MOTORS
            warmMotorCount.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Increased to 20ms for better cooling
            // If warm motors persist for 1000 cycles, temporarily disable force output
            if (warmMotorCount.load() > 1000) {
                std::fprintf(stderr, "[WARN] Persistent HD_WARM_MOTORS, disabling force output temporarily\n");
                hdDisable(HD_FORCE_OUTPUT);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Cool-down period
                hdEnable(HD_FORCE_OUTPUT);
                warmMotorCount.store(0); // Reset counter
            }
        } else {
            warmMotorCount.store(0); // Reset counter on other errors
        }
    } else {
        warmMotorCount.store(0); // Reset counter if no error
    }

    hdEndFrame(hdGetCurrentDevice());
    g_sampleCounter.fetch_add(1, std::memory_order_relaxed);

    return running.load(std::memory_order_relaxed) ? HD_CALLBACK_CONTINUE : HD_CALLBACK_DONE;
}

int haptic_init() {
    g_hHD = hdInitDevice(HD_DEFAULT_DEVICE);
    HDErrorInfo err = hdGetError();
    if (HD_DEVICE_ERROR(err)) {
        std::fprintf(stderr, "[ERR] Failed to initialize haptic device: Code=%d, Internal=%d\n",
                     err.errorCode, err.internalErrorCode);
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Increased to 1000ms for stabilization
    const char* deviceName = hdGetString(HD_DEVICE_MODEL_TYPE);
    std::fprintf(stderr, "[DEBUG] Device initialized: Model=%s\n",
                 deviceName ? deviceName : "Unknown");

    hdEnable(HD_FORCE_OUTPUT);
    return 0;
}

int haptic_start() {
    hdStartScheduler();
    HDErrorInfo err = hdGetError();
    if (HD_DEVICE_ERROR(err)) {
        std::fprintf(stderr, "[ERR] Failed to start scheduler: Code=%d, Internal=%d\n",
                     err.errorCode, err.internalErrorCode);
        return 1;
    }
    g_hCallback = hdScheduleAsynchronous(servoCallback, nullptr, HD_MAX_SCHEDULER_PRIORITY);
    std::fprintf(stderr, "[DEBUG] Scheduler started, callback scheduled\n");
    return 0;
}

void haptic_get_sample(struct Sample* sample) {
    sample->pos = g_lastSample.pos;
    sample->force = g_lastSample.force;
    sample->buttons = g_lastSample.buttons;
    sample->errorCode = g_lastSample.errorCode;
    sample->internalErrorCode = g_lastSample.internalErrorCode;
}

unsigned long long haptic_get_sample_count() {
    return g_sampleCounter.load();
}

void haptic_set_force(double force[3]) {
    if (g_hHD != HD_INVALID_HANDLE) {
        // Check device error state before setting force
        HDErrorInfo err = hdGetError();
        if (HD_DEVICE_ERROR(err) && err.errorCode == 1024) {
            std::fprintf(stderr, "[WARN] Skipping force set due to HD_WARM_MOTORS\n");
            return; // Skip force application if motors are warm
        }

        // Reduced max force to 2.0 N per axis to lessen motor strain
        const double max_force = 2.0;
        double clamped_force[3] = {
            std::max(std::min(force[0], max_force), -max_force),
            std::max(std::min(force[1], max_force), -max_force),
            std::max(std::min(force[2], max_force), -max_force)
        };
        hdBeginFrame(hdGetCurrentDevice());
        hdSetDoublev(HD_CURRENT_FORCE, clamped_force);
        hdEndFrame(hdGetCurrentDevice());
        err = hdGetError();
        if (HD_DEVICE_ERROR(err)) {
            g_lastSample.errorCode = err.errorCode;
            g_lastSample.internalErrorCode = err.internalErrorCode;
            std::fprintf(stderr, "[ERR] Failed to set force: Code=%d, Internal=%d, Force=[%f, %f, %f]\n",
                         err.errorCode, err.internalErrorCode, clamped_force[0], clamped_force[1], clamped_force[2]);
        }
    }
}

void haptic_stop() {
    running.store(false);
    if (g_hCallback != 0) {
        hdUnschedule(g_hCallback); // Unschedule first to prevent callback execution
        HDErrorInfo err = hdGetError();
        if (HD_DEVICE_ERROR(err)) {
            std::fprintf(stderr, "[ERR] Failed to unschedule callback: Code=%d, Internal=%d\n",
                         err.errorCode, err.internalErrorCode);
        }
        g_hCallback = 0;
    }
    if (g_hHD != HD_INVALID_HANDLE) {
        hdStopScheduler();
        HDErrorInfo err = hdGetError();
        if (HD_DEVICE_ERROR(err)) {
            std::fprintf(stderr, "[ERR] Failed to stop scheduler: Code=%d, Internal=%d\n",
                         err.errorCode, err.internalErrorCode);
        }
        hdDisableDevice(g_hHD);
        err = hdGetError();
        if (HD_DEVICE_ERROR(err)) {
            std::fprintf(stderr, "[ERR] Failed to disable device: Code=%d, Internal=%d\n",
                         err.errorCode, err.internalErrorCode);
        }
        g_hHD = HD_INVALID_HANDLE;
    }
    std::fprintf(stderr, "[DEBUG] Device stopped\n");
}
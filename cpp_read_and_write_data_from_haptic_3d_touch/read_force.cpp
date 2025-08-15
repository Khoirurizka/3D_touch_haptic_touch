// file: read_force.cpp
#include <HD/hd.h>
#include <HDU/hduVector.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

// Shared sample updated by the servo callback
struct Sample {
    hduVector3Dd pos;    // mm
    hduVector3Dd force;  // N (device-frame)
    HDint         buttons;
    HDErrorInfo   lastError;
};
static std::atomic<bool> running{true};
static Sample g_lastSample;
static std::atomic<uint64_t> g_sampleCounter{0};

// Real-time servo callback: read position & force
HDCallbackCode HDCALLBACK servoCallback(void* /*pUserData*/)
{
    hdBeginFrame(hdGetCurrentDevice());

    // Read current end-effector position (mm), force (N), and button state
    hduVector3Dd pos, force;
    HDint buttons = 0;
    hdGetDoublev(HD_CURRENT_POSITION, pos);
    hdGetDoublev(HD_CURRENT_FORCE,   force);
    hdGetIntegerv(HD_CURRENT_BUTTONS, &buttons);

    // Store into the shared sample (lightweight copy)
    g_lastSample.pos = pos;
    g_lastSample.force = force;
    g_lastSample.buttons = buttons;

    // Catch any errors that occurred this frame (optional)
    HDErrorInfo err = hdGetError();
    g_lastSample.lastError = err;

    hdEndFrame(hdGetCurrentDevice());

    g_sampleCounter.fetch_add(1, std::memory_order_relaxed);
    return running.load(std::memory_order_relaxed) ? HD_CALLBACK_CONTINUE : HD_CALLBACK_DONE;
}

int main()
{
    // 1) Open device
    HHD hHD = hdInitDevice(HD_DEFAULT_DEVICE);
    if (HD_DEVICE_ERROR(hdGetError()))
    {
        std::fprintf(stderr, "[ERR] Failed to initialize haptic device. Check driver/cable.\n");
        return 1;
    }

    // 2) Enable force output (usually already on with HDAPI, but explicit is fine)
    hdEnable(HD_FORCE_OUTPUT);

    // 3) Start the scheduler and schedule our callback
    hdStartScheduler();
    if (HD_DEVICE_ERROR(hdGetError()))
    {
        std::fprintf(stderr, "[ERR] Failed to start scheduler.\n");
        return 1;
    }

HDSchedulerHandle hCallback =
    hdScheduleAsynchronous(servoCallback, nullptr, HD_MAX_SCHEDULER_PRIORITY);


    // 4) Poll at ~100 Hz from the app thread and print latest values
    std::printf("Reading Touch force/position... press Ctrl+C to stop.\n");
    while (!HD_DEVICE_ERROR(hdGetError()) && running.load())
    {
        // Copy the latest sample and print
        Sample s = g_lastSample;

        // Check if any device error was reported in the last servo frame
        if (HD_DEVICE_ERROR(s.lastError))
        {
            std::fprintf(stderr, "[HD ERR] Code=%d, internal=%d\n",
                         s.lastError.errorCode, s.lastError.internalErrorCode);
        }

        std::printf("pos [mm]: %+8.3f %+8.3f %+8.3f | force [N]: %+6.3f %+6.3f %+6.3f | buttons: 0x%X | samples: %llu\r",
                    s.pos[0], s.pos[1], s.pos[2],
                    s.force[0], s.force[1], s.force[2],
                    s.buttons,
                    static_cast<unsigned long long>(g_sampleCounter.load()));
        std::fflush(stdout);

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // ~100 Hz print
    }

    // 5) Clean shutdown
    running.store(false);
    hdStopScheduler();
    hdUnschedule(hCallback);
    hdDisableDevice(hHD);
    std::puts("\nStopped.");
    return 0;
}


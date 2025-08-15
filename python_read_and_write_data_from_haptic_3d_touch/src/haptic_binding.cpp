#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "haptic_lib.h"

namespace py = pybind11;

PYBIND11_MODULE(haptic, m) {
    // Define the Sample struct
    py::class_<Sample>(m, "Sample")
        .def(py::init<>())
        .def_readwrite("pos", &Sample::pos)
        .def_readwrite("force", &Sample::force)
        .def_readwrite("buttons", &Sample::buttons)
        .def_readwrite("errorCode", &Sample::errorCode)
        .def_readwrite("internalErrorCode", &Sample::internalErrorCode);

    // Bind the C functions
    m.def("init", &haptic_init, "Initialize the haptic device");
    m.def("start", &haptic_start, "Start the haptic servo loop");
    m.def("get_sample", [](py::object sample) {
        Sample* s = sample.cast<Sample*>();
        haptic_get_sample(s);
    }, "Get the latest haptic sample", py::arg("sample"));
    m.def("get_sample_count", &haptic_get_sample_count, "Get the sample counter");
    m.def("set_force", [](std::array<double, 3> force) {
        haptic_set_force(force.data());
    }, "Set the haptic device force", py::arg("force"));
    m.def("stop", &haptic_stop, "Stop and cleanup the haptic device");
}
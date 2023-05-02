#include "init.hpp"

namespace vmgs {
    pybinder_t::binder_map_t& pybinder_t::registered_binders() {
        static binder_map_t binders;
        return binders;
    }
}

PYBIND11_MODULE(_vmgs, m) {
    py::register_local_exception_translator([](std::exception_ptr ep) {
        try {
            if (ep) {
                std::rethrow_exception(ep);
            }
        } catch (std::system_error& e) {
#if defined(WIN32)
            if (e.code().category() == std::system_category()) {
                PyErr_SetExcFromWindowsErr(PyExc_WindowsError, e.code().value());
#else
            if (e.code().category() == std::generic_category()) {
                PyErr_SetFromErrno(PyExc_OSError);
#endif
            } else {
                PyErr_SetString(PyExc_RuntimeError, e.what());
            }
        }

    });

    for (auto& [_, binder] : vmgs::pybinder_t::registered_binders()) {
        binder->declare(m);
    }

    for (auto& [_, binder] : vmgs::pybinder_t::registered_binders()) {
        binder->make_binding(m);
    }
}

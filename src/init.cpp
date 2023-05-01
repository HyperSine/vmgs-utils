#include "init.hpp"

#if defined(WIN32)
#include <wil/result.h>
#endif

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
        }
#if defined(WIN32)
        catch (wil::ResultException& e) {
            PyErr_SetExcFromWindowsErr(PyExc_WindowsError, e.GetErrorCode());
        }
#else
        catch (std::system_error& e) {
            if (e.code().category() == std::generic_category()) {
                PyErr_SetFromErrno(PyExc_OSError);
            } else {
                PyErr_SetString(PyExc_RuntimeError, e.what());
            }
        }
#endif
    });

    for (auto& [_, binder] : vmgs::pybinder_t::registered_binders()) {
        binder->declare(m);
    }

    for (auto& [_, binder] : vmgs::pybinder_t::registered_binders()) {
        binder->make_binding(m);
    }
}

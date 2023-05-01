#include "init.hpp"

#if __has_include(<wil/result.h>)
#include <wil/result.h>
#endif

namespace vmgs {
    pybinder_t::binder_map_t& pybinder_t::registered_binders() {
        static binder_map_t binders;
        return binders;
    }
}

PYBIND11_MODULE(vmgs, m) {
#if __has_include(<wil/result.h>)
    py::register_local_exception_translator([](std::exception_ptr ep) {
        try {
            if (ep) {
                std::rethrow_exception(ep);
            }
        } catch (wil::ResultException& e) {
            PyErr_SetExcFromWindowsErr(PyExc_WindowsError, e.GetErrorCode());
        }
    });
#endif

    for (auto& [_, binder] : vmgs::pybinder_t::registered_binders()) {
        binder->declare(m);
    }

    for (auto& [_, binder] : vmgs::pybinder_t::registered_binders()) {
        binder->make_binding(m);
    }
}

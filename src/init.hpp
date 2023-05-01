#pragma once
#include "py.hpp"

namespace vmgs {
    struct pybinder_t {
        using binder_map_t = std::map<std::string_view, pybinder_t*>;

        [[nodiscard]]
        static binder_map_t& registered_binders();

        virtual void declare(py::module_& m) = 0;
        virtual void make_binding(py::module_& m) = 0;
    };

    template<typename T>
    struct enum_pybinder_t;

    template<typename T>
    struct class_pybinder_t;

    template<typename Tag>
    struct function_pybinder_t;
}

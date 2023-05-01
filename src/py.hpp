#pragma once
#include <type_traits>
#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_RUNTIME_EXCEPTION(not_implemented_error, PyExc_NotImplementedError)

template<typename T>
struct nullable_ptr {
    std::add_pointer_t<T> value;
};

template<typename T>
struct nonnull_ptr {
    std::add_pointer_t<T> value;
};

class Any : public object {
private:
    static int _Check(PyObject*) { return 1; }

public:
    PYBIND11_OBJECT(Any, object, _Check)
};

class BinaryIO : public object {
private:
    static int _Check(PyObject*) {
        return 1;   // not implemented yet
    }

public:
    PYBIND11_OBJECT(BinaryIO, object, _Check)
};

template <typename Signature>
class Callable;

template <typename Return, typename... Args>
class Callable<Return(Args...)> : public function {
    using function::function;
};

PYBIND11_NAMESPACE_BEGIN(detail)
template<typename Type>
struct type_caster<nullable_ptr<Type>> {
    using value_conv = make_caster<decltype(nullable_ptr<Type>::value)>;
    PYBIND11_TYPE_CASTER(nullable_ptr<Type>, const_name("Optional[") + value_conv::name + const_name("]"));

    bool load(handle src, bool convert) {
        value_conv conv;

        if (src.is_none()) {
            value = nullable_ptr<Type>{ nullptr };
            return true;
        }

        if (conv.load(src, convert)) {
            value = nullable_ptr<Type>{ std::move(conv) };
            return true;
        } else {
            return false;
        }
    }

    static handle cast(nullable_ptr<Type> src, return_value_policy policy, handle parent) {
        return value_conv::cast(src.value, policy, parent);
    }
};

template<typename Type>
struct type_caster<nonnull_ptr<Type>> {
    using value_conv = make_caster<decltype(nonnull_ptr<Type>::value)>;
    PYBIND11_TYPE_CASTER(nonnull_ptr<Type>, value_conv::name);

    bool load(handle src, bool convert) {
        value_conv conv;
        if (!src.is_none() && conv.load(src, convert)) {
            value = nonnull_ptr<Type>{ std::move(conv) };
        } else {
            return false;
        }
    }

    static handle cast(nonnull_ptr<Type> src, return_value_policy policy, handle parent) {
        return value_conv::cast(src.value, policy, parent);
    }
};

template<>
struct handle_type_name<Any> {
    static constexpr auto name = const_name("Any");
};

template<>
struct handle_type_name<BinaryIO> {
    static constexpr auto name = const_name("BinaryIO");
};

template <typename Return, typename... Args>
struct handle_type_name<Callable<Return(Args...)>> {
    using retval_type = conditional_t<std::is_same<Return, void>::value, void_type, Return>;
    static constexpr auto name = 
        const_name("Callable[[") + concat(make_caster<Args>::name...) + const_name("], ") + make_caster<retval_type>::name + const_name("]");
};
PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)

namespace py = pybind11;

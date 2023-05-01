#pragma once
#include <concepts>
#include <type_traits>

namespace vmgs {
    namespace concepts {
        template<typename Ty>
        concept arithmetic = std::is_arithmetic_v<Ty>;

        template<typename Ty>
        concept fundamental = std::is_fundamental_v<Ty>;

        template<typename Ty>
        concept byte_compatible = std::integral<Ty> && sizeof(Ty) == 1 || std::same_as<Ty, std::byte>;

        template<typename Ty, typename... Tys>
        concept one_of = std::disjunction_v<std::is_same<Ty, Tys>...>;

        template<typename Ty, typename... Tys>
        concept any_of = one_of<Ty, Tys...>;

        template<template<typename Ty> typename TraitTy, typename... Tys>
        concept all_satisfy = std::conjunction_v<TraitTy<Tys>...>;

        template<template<typename Ty> typename TraitTy, typename... Tys>
        concept none_satisfy = std::conjunction_v<std::negation<TraitTy<Tys>>...>;

        template<template<typename Ty> typename TraitTy, typename... Tys>
        concept some_satisfy = !none_satisfy<TraitTy, Tys...>;

        template<template<typename Ty> typename TraitTy, typename... Tys>
        concept some_unsatisfy = !all_satisfy<TraitTy, Tys...>;

        template<typename Ty, template<typename> typename... TraitTys>
        concept satisfy_all = std::conjunction_v<TraitTys<Ty>...>;

        template<typename Ty, template<typename> typename... TraitTys>
        concept satisfy_none = std::conjunction_v<std::negation<TraitTys<Ty>>...>;

        template<typename Ty, template<typename> typename... TraitTys>
        concept satisfy_some = !satisfy_none<Ty, TraitTys...>;

        template<typename Ty, template<typename> typename... TraitTys>
        concept unsatisfy_some = !satisfy_all<Ty, TraitTys...>;
    }
}

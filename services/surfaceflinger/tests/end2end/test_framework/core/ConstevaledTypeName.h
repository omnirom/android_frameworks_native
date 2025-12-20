/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <array>
#include <cstdlib>
#include <string_view>
#include <tuple>
#include <utility>

namespace android::surfaceflinger::tests::end2end::test_framework::core {
namespace detail {

class TypeNameOf {
    template <typename T>
    static consteval auto pretty() -> std::string_view {
        return __PRETTY_FUNCTION__;
    }

    static consteval auto trim(std::string_view name) -> std::string_view {
        constexpr std::string_view expected = "std::string_view";
        constexpr std::string_view ref = pretty<std::string_view>();
        constexpr size_t prefixLength = ref.rfind(expected);
        static_assert(prefixLength != std::string_view::npos);
        constexpr size_t suffixLength = ref.size() - prefixLength - expected.size();
        static_assert(prefixLength + suffixLength + expected.size() == ref.size());

        return name.substr(prefixLength, name.size() - suffixLength - prefixLength);
    }

    static consteval auto trimNamespace(std::string_view name) -> std::string_view {
        if (auto prefix = name.rfind("::"); prefix != std::string_view::npos) {
            name = name.substr(prefix + 2);
        }
        return name;
    }

    template <size_t... Indices>
    static consteval auto copy(std::string_view input, std::index_sequence<Indices...> sequence)
            -> std::array<char, 1 + sizeof...(Indices)> {
        std::ignore = sequence;
        return {input[Indices]..., 0};
    }

  public:
    template <typename T>
    static consteval auto fullyQualifiedName() -> std::string_view {
        constexpr auto name = trim(pretty<T>());
        static constexpr auto copied = copy(name, std::make_index_sequence<name.size()>{});
        return copied.data();
    }

    template <typename T>
    static consteval auto shortName() -> std::string_view {
        constexpr auto name = trimNamespace(trim(pretty<T>()));
        static constexpr auto copied = copy(name, std::make_index_sequence<name.size()>{});
        return copied.data();
    }
};

}  // namespace detail

template <typename T>
consteval auto typeNameOf() -> std::string_view {
    static constexpr auto name = detail::TypeNameOf::fullyQualifiedName<T>();
    return name;
}

template <typename T>
consteval auto shortTypeNameOf() -> std::string_view {
    static constexpr auto name = detail::TypeNameOf::shortName<T>();
    return name;
}

static_assert(typeNameOf<std::string_view>() == "std::string_view");
static_assert(typeNameOf<detail::TypeNameOf>() ==
              "android::surfaceflinger::tests::end2end::test_framework::core::detail::TypeNameOf");

static_assert(shortTypeNameOf<std::string_view>() == "string_view");
static_assert(shortTypeNameOf<detail::TypeNameOf>() == "TypeNameOf");

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core

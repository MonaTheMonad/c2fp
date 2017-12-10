#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#define I_KNOW_WHAT_IM_DOING
#include <detail.hpp>
#undef I_KNOW_WHAT_IM_DOING

namespace c2fp {
    template <typename R, typename... Args>
    class closure {
      public: // member types
        using type = R(Args...);
        using pointer_type = type*;
        using reference_type = type&; // Why would you ever need this?
        
      public:
        static constexpr size_t arg_count = sizeof...(Args);
        
      private: // members
        pointer_type fptr;
        size_t size;
        
      public: // (de)constructors
        template <typename Function>
        closure(Function&& f) : fptr(nullptr) {
            auto op = &std::decay<Function>::type::operator();
            void* raw_pf = reinterpret_cast<void*&>(op);
            void* target = &f;
            void* pf = detail::build_function(raw_pf, target, static_cast<uint32_t>(arg_count), &size);
            fptr = reinterpret_cast<pointer_type>(pf);
        }
        closure(const closure&) = delete;
        closure(closure&& c) : fptr(c.fptr) {
            c.fptr = nullptr;
        }
        compl closure() {
            if (fptr)
                detail::delete_function(reinterpret_cast<void*>(fptr));
        }
        
      public: // member functions
        pointer_type function_pointer() const {
            return fptr;
        }
        // Why would you ever need this?
        reference_type function_reference() const {
            return fptr[0];
        }
        size_t function_size() const {
            return size;
        }
    };
}

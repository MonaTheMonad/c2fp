#pragma once

#ifdef I_KNOW_WHAT_IM_DOING
# undef I_KNOW_WHAT_IM_DOING
#else
# error "Don't include this header!"
#endif

namespace c2fp {
    namespace detail {
        using byte_vector = std::vector<uint8_t>;
        
        enum : uint8_t {
            rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
            r8,  r9,  r10, r11, r12, r13, r14, r15
        };
        
        class function_builder {
          private:
            byte_vector fbuf;
            
          public:
            void* build(void* func, void* target, uint32_t argc);
            void* build() const;
            size_t position() const;
            
          public:
            void inject_qword(size_t index, uint64_t value);
            void emit_move_arg(uint32_t from, uint32_t to, uint32_t stack_size);
            void emit_movq_rr(uint8_t from, uint8_t to);
            void emit_movq_rd(uint8_t from, uint32_t disp);
            void emit_movq_dr(uint32_t disp, uint8_t to);
            void emit_movq_dd(uint32_t from_disp, uint32_t to_disp, uint8_t scratch = rax);
            void emit_movq_vr(uint64_t value, uint8_t rm);
            void emit_add(uint8_t rm, uint32_t value);
            void emit_sub(uint8_t rm, uint32_t value);
            void emit_jmp(uint8_t rm);
            void emit_call(uint8_t rm);
            void emit_ret();
            void emit_rex(bool w, bool r, bool x, bool b);
            void emit_modrm(uint8_t mod, uint8_t reg, uint8_t rm);
            void emit_sib(uint8_t scale, uint8_t index, uint8_t base);
            void emit(uint8_t value);
            void emit_dword(uint32_t value);
            void emit_qword(uint64_t value);
        };
        
        void* build_function(void* func, void* target, uint32_t argc, size_t* size = nullptr);
        void delete_function(void* func);
    }
}

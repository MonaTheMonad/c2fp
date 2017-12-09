#include <algorithm>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#include <c2fp.hpp>

using namespace std;

namespace c2fp {
    namespace detail {
        static constexpr uint32_t max_reg_args = 6;
        static constexpr int all_permissions = PROT_READ | PROT_WRITE | PROT_EXEC;
        static const size_t page_size = (size_t) (uint32_t) getpagesize();
        
        template <typename Iterator>
        static void debug_print(const char* message, Iterator it, size_t len) {
            cout << message << ' ';
            for (size_t i = 0; i < len; i++)
                cout << setw(2) << (uint32_t) it[i] << ' ';
            cout << endl;
        }
        
        static void debug_print(uint64_t from, uint64_t to) {
            cout << "    " << setw(16) << from << " -> " << setw(16) << to << endl;
        }
        
        static uint8_t arg_to_reg(uint32_t arg) {
            switch (arg) {
                case 0: return rdi;
                case 1: return rsi;
                case 2: return rdx;
                case 3: return rcx;
                case 4: return r8;
                case 5: return r9;
                default:
                    throw runtime_error("Argument number exceeds maximum number of argument registers."); // Should never happen
            }
        }
        
        static int32_t arg_to_disp(uint32_t arg) {
            if (arg < max_reg_args)
                throw runtime_error("Argument number cannot be less than maximum number of argument registers."); // Should never happen
            return (arg - max_reg_args) * 8;
        }
        
        static int32_t arg_to_arg_disp(uint32_t arg, uint32_t stack_size) {
            return arg_to_disp(arg) + stack_size + 8;
        }
        
        static uint32_t compute_extra_args(uint32_t argc) {
            const int32_t extras = argc - max_reg_args;
            return extras < 0 ? 0 : extras;
        }
        
        static uint32_t compute_stack_size(uint32_t extra_args) {
            const bool even = extra_args % 2 == 0;
            return (even ? extra_args + 1 : extra_args) * 8;
        }
        
        static void* align_to_page(void* p) {
            uintptr_t& i = reinterpret_cast<uintptr_t&>(p);
            return reinterpret_cast<void*>(i / page_size * page_size);
        }
        
        void* build_function(void* func, void* target, uint32_t argc, size_t* size) {
            function_builder builder;
            func = builder.build(func, target, argc);
            if (size)
                size[0] = builder.position();
            return func;
        }
        
        void delete_function(void* func) {
            delete[] reinterpret_cast<char*>(func);
        }
        
        void* function_builder::build(void* func, void* target, uint32_t argc) {
            const uint32_t extra_args = compute_extra_args(argc + 1);
            const uint32_t stack_size = compute_stack_size(extra_args);
            emit_sub(rsp, stack_size);
            for (int i = argc - 1; i >= 0; i--)
                emit_move_arg(i, i + 1, stack_size);
            emit_movq_vr(reinterpret_cast<uint64_t>(func), rax);
            emit_movq_vr(reinterpret_cast<uint64_t>(target), rdi);
            emit_call(rax);
            emit_add(rsp, stack_size);
            emit_ret();
            return build();
        }
        
        void* function_builder::build() const {
            char* fbytes = new char[fbuf.size()];
            copy(fbuf.begin(), fbuf.end(), fbytes);
            if (mprotect(align_to_page(fbytes), page_size, all_permissions)) {
                delete[] fbytes;
                throw runtime_error("Could not make function executable.");
            }
            return reinterpret_cast<void*>(fbytes);
        }
        
        size_t function_builder::position() const {
            return fbuf.size();
        }
        
        void function_builder::inject_qword(size_t index, uint64_t value) {
            copy(&value, &value + 1, fbuf.begin() + index);
        }
        
        void function_builder::emit_move_arg(uint32_t from, uint32_t to, uint32_t stack_size) {
            const bool is_from_reg = from < max_reg_args;
            const bool is_to_reg = to < max_reg_args;
            if (is_from_reg) {
                if (is_to_reg)
                    emit_movq_rr(arg_to_reg(from), arg_to_reg(to));
                else
                    emit_movq_rd(arg_to_reg(from), arg_to_disp(to));
            } else {
                if (is_to_reg)
                    emit_movq_dr(arg_to_arg_disp(from, stack_size), arg_to_reg(to));
                else
                    emit_movq_dd(arg_to_arg_disp(from, stack_size), arg_to_disp(to));
            }
        }
        
        void function_builder::emit_movq_rr(uint8_t from, uint8_t to) {
            emit_rex(true, from & 8, false, to & 8);
            emit(0x89);
            emit_modrm(3, from, to);
            debug_print("movqrr", fbuf.end() - 3, 3);
            debug_print(from, to);
        }
        
        void function_builder::emit_movq_rd(uint8_t from, uint32_t disp) {
            emit_rex(true, from & 8, false, false);
            emit(0x89);
            emit_modrm(2, from, rsp);
            emit_sib(0, rsp, rsp);
            emit_dword(disp);
            debug_print("movqrd", fbuf.end() - 7, 7);
            debug_print(from, disp);
        }
        
        void function_builder::emit_movq_dr(uint32_t disp, uint8_t to) {
            emit_rex(true, false, false, to & 8);
            emit(0x8B);
            emit_modrm(2, to, rsp);
            emit_sib(0, rsp, rsp);
            emit_dword(disp);
            debug_print("movqdr", fbuf.end() - 7, 7);
            debug_print(disp, to);
        }
        
        void function_builder::emit_movq_dd(uint32_t from_disp, uint32_t to_disp, uint8_t scratch) {
            emit_movq_dr(from_disp, scratch);
            emit_movq_rd(scratch, to_disp);
        }
        
        void function_builder::emit_movq_vr(uint64_t value, uint8_t reg) {
            emit_rex(true, false, false, reg & 8);
            emit(0xB8 + reg);
            emit_qword(value);
            debug_print("movqvr", fbuf.end() - 10, 10);
            debug_print(value, reg);
        }
        
        void function_builder::emit_add(uint8_t rm, uint32_t value) {
            emit_rex(true, false, false, rm & 8);
            emit(0x81);
            emit_modrm(3, 0, rm);
            emit_dword(value);
            debug_print("add   ", fbuf.end() - 7, 7);
            debug_print(rm, value);
        }
        
        void function_builder::emit_sub(uint8_t rm, uint32_t value) {
            emit_rex(true, false, false, rm & 8);
            emit(0x81);
            emit_modrm(3, 5, rm);
            emit_dword(value);
            debug_print("sub   ", fbuf.end() - 7, 7);
            debug_print(rm, value);
        }
        
        void function_builder::emit_jmp(uint8_t rm) {
            emit(0xFF);
            emit_modrm(3, 4, rm);
            debug_print("jmp   ", fbuf.end() - 2, 2);
            debug_print(rm, 0);
        }
        
        void function_builder::emit_call(uint8_t rm) {
            emit(0xFF);
            emit_modrm(3, 2, rm);
            debug_print("call  ", fbuf.end() - 2, 2);
            debug_print(rm, 0);
        }
        
        void function_builder::emit_ret() {
            emit(0xC3);
            debug_print("ret   ", fbuf.end() - 1, 1);
        }
        
        void function_builder::emit_rex(bool w, bool r, bool x, bool b) {
            const uint8_t wrxb = (w ? 8 : 0)
                               | (r ? 4 : 0)
                               | (x ? 2 : 0)
                               | (b ? 1 : 0);
            emit(0x40 | wrxb);
        }
        
        void function_builder::emit_modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
            mod <<= 6;
            reg = static_cast<uint8_t>(reg & 7) << 3;
            rm &= static_cast<uint8_t>(7);
            emit(mod | reg | rm);
        }
        
        void function_builder::emit_sib(uint8_t scale, uint8_t index, uint8_t base) {
            emit_modrm(scale, index, base);
        }
        
        void function_builder::emit(uint8_t value) {
            fbuf.push_back(value);
        }
        
        void function_builder::emit_dword(uint32_t value) {
            union {
                uint32_t i;
                struct {
                    uint8_t a, b, c, d;
                };
            } u = {value};
            emit(u.a);
            emit(u.b);
            emit(u.c);
            emit(u.d);
        }
        
        void function_builder::emit_qword(uint64_t value) {
            union {
                uint64_t i;
                struct {
                    uint8_t a, b, c, d, e, f, g, h;
                };
            } u = {value};
            emit(u.a);
            emit(u.b);
            emit(u.c);
            emit(u.d);
            emit(u.e);
            emit(u.f);
            emit(u.g);
            emit(u.h);
        }
    }
}

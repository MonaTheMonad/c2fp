#include <functional>
#include <iomanip>
#include <iostream>
#include <c2fp.hpp>

using namespace std;
using namespace c2fp;

template <typename F>
struct closure_traits;

template <typename R, typename T, typename... Args>
struct closure_traits<R (T::*)(Args...)> {
	using function_type = R (*)(Args...);
};

template <typename T>
static void inspect(const T& t) {
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&t);
	for (size_t i = 0; i < sizeof(T); i++) {
		cout << setw(2) << (unsigned int) bytes[i] << ' ';
		if ((i + 1) % 8 == 0)
			cout << '\n';
	}
	cout << endl;
}

template <typename R, typename... Args>
static void inspect(const closure<R, Args...>& c) {
	const size_t size = c.function_size();
	const auto fptr = c.function_pointer();
	const unsigned char* fbytes = reinterpret_cast<const unsigned char*>(fptr);
	for (size_t i = 0; i < size; i++) {
		cout << setw(2) << (unsigned int) fbytes[i] << ' ';
		if ((i + 1) % 8 == 0)
			cout << '\n';
	}
	cout << endl;
}

template <typename R, typename T, typename... Args>
static constexpr auto member_function_to_function(R (T::*pmf)(Args...) const) -> R (*)(T&, Args...) {
	return reinterpret_cast<R (*&)(T&, Args...)>(pmf);
}

static void hello_world() {
	cout << "Hello world!" << endl;
}

static void raw_emission_a(void* pf, void* pt) {
	using namespace c2fp::detail;
	function_builder builder;
	builder.emit_sub(rsp, 8);
	// builder.emit_movq_rr(rdx, rcx);
	// builder.emit_movq_rr(rsi, rdx);
	// builder.emit_movq_rr(rdi, rsi);
	// builder.emit_movq_vr(reinterpret_cast<uint64_t>(pf), rax);
	// builder.emit_movq_vr(reinterpret_cast<uint64_t>(pt), rdi);
	builder.emit_movq_vr(reinterpret_cast<uint64_t>(&hello_world), rax);
	builder.emit_call(rax);
	builder.emit_add(rsp, 8);
	builder.emit_ret();
	void* p = builder.build();
	reinterpret_cast<void (*)()>(p)();
	delete[] reinterpret_cast<char*>(p);
}

static void raw_emission_b(void* pf, void* pt) {
	using namespace c2fp::detail;
	using convoluted_function_t = void (*)(const char*, int, const char*, const char*, const char*, const char*, const char*);
	uint64_t& pfi = reinterpret_cast<uint64_t&>(pf);
	uint64_t& pti = reinterpret_cast<uint64_t&>(pt);
	function_builder fb;
	fb.emit_sub(rsp, 0x18);
	fb.emit_movq_dd(0x20, 0x08); // 7th to 8th
	// fb.emit_movq_rd(r9, 0x00); // 6th to 7th
	// fb.emit_movq_rr(r8, r9);
	// fb.emit_movq_rr(rcx, r8);
	// fb.emit_movq_rr(rdx, rcx);
	// fb.emit_movq_rr(rsi, rdx);
	// fb.emit_movq_rr(rdi, rsi);
	fb.emit_add(rsp, 0x18);
	fb.emit_ret();
	reinterpret_cast<convoluted_function_t>(fb.build())("CKMo", 2, "apples", "foo", "bar", "baz", "qux");
}

static void init() {
	cout << boolalpha << hex << uppercase << setfill('0');
}

int main() {
	function<void(const char*, int, const char*, const char*, const char*, const char*, const char*)> f = [](const char* who, int num, const char* what, const char* foo, const char* bar, const char* baz, const char* qux) {
		cout << who << " has " << num << " " << what << '\n';
		cout << foo << bar << baz << qux << endl;
	};
	init();
	// auto pf = member_function_to_function(&decltype(f)::operator());
	// void* pt = &f;
	closure<void, const char*, int, const char*, const char*, const char*, const char*, const char*> c(f);
	// c.closure_function<decltype(f)>()(c.closure_target<decltype(f)>(), "CKMo", 2, "apples");
	// inspect(c);
	c.function_pointer()("CKMo", 2, "apples", "foo", "bar", "baz", "qux");
	// member_function_to_function(&decltype(f)::operator())(f, "CKMo", 2, "apples");
	// void* pf = reinterpret_cast<void*>(member_function_to_function(&decltype(f)::operator()));
	// void* pt = reinterpret_cast<void*>(&f);
	// cout << setw(16) << reinterpret_cast<uintptr_t>(pf) << endl;
	// cout << setw(16) << reinterpret_cast<uintptr_t>(pt) << endl;
	// reinterpret_cast<void (*)(decltype(f)&, const char*, int, const char*)>(pf)(f, "CKMo", 2, "apples");
	// raw_emission(pf, pt);
	// raw_emission_b(reinterpret_cast<void*>(member_function_to_function(&decltype(f)::operator())), &f);
	return 0;
}

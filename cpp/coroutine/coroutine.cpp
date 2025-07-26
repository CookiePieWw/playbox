#include <coroutine>
#include <generator>
#include <iostream>

template <typename T> struct Generator {
	struct promise_type;
	explicit constexpr Generator(std::coroutine_handle<promise_type> h) noexcept
		: _handle(h) {}
	~Generator() { _handle ? _handle.destroy() : void(0); }
	Generator(const Generator &) = delete;
	Generator(Generator &&rhs) = delete;
	Generator &operator=(const Generator &rhs) = delete;

	T operator()() {
		if (!resumable()) {
			throw std::runtime_error("AIEEEEE! A NINJA!?");
		}
		if (!operator bool()) {
			throw std::logic_error("WHY THERE'S A NINJA HERE!?");
		}
		_handle.resume();
		_handle.promise().count_--;
		return std::move(_handle.promise().result_);
	}

	explicit operator bool() const noexcept {
		return _handle.promise().count_ > 0;
	}

	bool resumable() const noexcept { return _handle && !_handle.done(); }

  private:
	std::coroutine_handle<promise_type> _handle;
};

template <typename T> struct Generator<T>::promise_type {
	explicit constexpr promise_type(size_t count) noexcept(noexcept(T()))
		: count_(count) {}

	auto initial_suspend() noexcept { return std::suspend_always{}; }
	auto final_suspend() noexcept { return std::suspend_never{}; }
	auto unhandled_exception() {}

	auto get_return_object() noexcept {
		auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
		return Generator<T>(handle);
	}

	auto yield_value(auto &&value) noexcept {
		result_ = std::forward<decltype(value)>(value);
		return std::suspend_always{};
	}

	size_t count_;
	T result_;
};

Generator<double> async_fib(size_t n) {
	auto a = 0.1, b = 0.2;
	while (true) {
		co_yield b;
		b = a + b;
		a = b - a;
	}
}

int main() {
	auto gen = async_fib(10);
	while (gen) {
		std::cout << gen() << std::endl;
	}
	return 0;
}

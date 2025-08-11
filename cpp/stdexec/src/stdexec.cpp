#include <iostream>
#include <stdexec/execution.hpp>
#include <thread>

namespace stdex = stdexec;

stdex::run_loop loop;
std::thread worker([] { loop.run(); });

int main() {
	stdex::scheduler auto scheduler = loop.get_scheduler();
	stdex::sender auto print = stdex::schedule(scheduler) |
							   stdex::then([] { return "Hello, World!\n"; }) |
							   stdex::then([](const auto msg) {
								   std::cout << msg;
								   return 0;
							   });

	auto [result] = stdex::sync_wait(print).value();

	loop.finish();
	worker.join();
	return result;
}
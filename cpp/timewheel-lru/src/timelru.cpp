#include "timelru.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

template struct TimeLRUCache<std::string, std::string, 10>;

template <typename Key, typename Value, size_t SlotNum>
std::thread StartTimer(TimeLRUCache<Key, Value, SlotNum> &cache) {
	std::thread timerThread([&cache]() { cache.Start(); });
	return timerThread;
}

void testBasicTimeLRUOperations() {
	std::cout << "=== Testing Basic TimeLRU Operations ===" << std::endl;
	TimeLRUCache<std::string, std::string, 10> cache;
	auto thread = StartTimer(cache);

	// Test Put and Get (intervals in seconds)
	cache.Put("key1", "value1", 10); // 10 seconds
	cache.Put("key2", "value2", 20); // 20 seconds
	cache.Put("key3", "value3", 30); // 30 seconds

	assert(cache.Size() == 3);

	auto value = cache.Get("key1");
	assert(value != nullptr);
	assert(*value == "value1");

	value = cache.Get("key2");
	assert(value != nullptr);
	assert(*value == "value2");

	value = cache.Get("nonexistent");
	assert(value == nullptr);

	std::cout << "Basic TimeLRU operations test passed!" << std::endl;
	cache.Stop();
	thread.join();
}

void testTimeLRUTryPut() {
	std::cout << "\n=== Testing TimeLRU TryPut ===" << std::endl;
	TimeLRUCache<std::string, std::string, 10> cache;
	auto thread = StartTimer(cache);

	bool result = cache.TryPut("key1", "value1", 6); // 6 seconds timeout
	assert(result == true);

	result = cache.TryPut("key1", "value1_updated", 8); // 8 seconds timeout
	assert(result == false);

	auto value = cache.Get("key1");
	assert(value != nullptr);
	assert(*value == "value1"); // Should still be original value

	std::cout << "TimeLRU TryPut test passed!" << std::endl;
	cache.Stop();
	thread.join();
}

void testTimeLRUManualEviction() {
	std::cout << "\n=== Testing TimeLRU Manual Eviction ===" << std::endl;
	TimeLRUCache<std::string, std::string, 10> cache;
	auto thread = StartTimer(cache);

	cache.Put("key1", "value1", 5); // 5 seconds
	cache.Put("key2", "value2", 7); // 7 seconds
	cache.Put("key3", "value3", 9); // 9 seconds

	assert(cache.Size() == 3);

	// Test specific key eviction
	cache.Evict("key2");
	assert(cache.Size() == 2);

	auto value = cache.Get("key2");
	assert(value == nullptr);

	value = cache.Get("key1");
	assert(value != nullptr);
	assert(*value == "value1");

	// Test general eviction (LRU)
	cache.Evict();
	assert(cache.Size() == 1);

	std::cout << "TimeLRU manual eviction test passed!" << std::endl;
	cache.Stop();
	thread.join();
}

void testTimeLRUThreadSafety() {
	std::cout << "\n=== Testing TimeLRU Thread Safety ===" << std::endl;
	TimeLRUCache<int, std::string, 10> cache;
	auto thread = StartTimer(cache);

	// Test concurrent access (intervals within max limit of 10 seconds)
	std::thread writer1([&cache]() {
		for (int i = 0; i < 50; ++i) {
			cache.Put(i, "value" + std::to_string(i), 10); // 10 seconds
		}
	});

	std::thread writer2([&cache]() {
		for (int i = 50; i < 100; ++i) {
			cache.TryPut(i, "value" + std::to_string(i), 8); // 8 seconds
		}
	});

	std::thread reader([&cache]() {
		for (int i = 0; i < 100; ++i) {
			(void)cache.Get(i); // Just accessing for thread safety test
		}
	});

	writer1.join();
	writer2.join();
	reader.join();

	// At least some items should be in cache
	assert(cache.Size() > 0);

	std::cout << "TimeLRU thread safety test passed!" << std::endl;
	cache.Stop();
	thread.join();
}

void testTimeLRUTimeBasedEviction() {
	std::cout << "\n=== Testing TimeLRU Time-based Eviction ===" << std::endl;
	TimeLRUCache<std::string, std::string, 10> cache;
	auto thread = StartTimer(cache);

	// Add items with short and long timeouts (within 10 second max)
	cache.Put("short1", "value1", 2); // 2 seconds timeout
	cache.Put("short2", "value2", 2); // 2 seconds timeout
	cache.Put("long1", "value3", 8);  // 8 seconds timeout

	assert(cache.Size() == 3);

	// Wait for short timeouts to expire
	std::this_thread::sleep_for(std::chrono::seconds(3));

	auto value1 = cache.Get("short1");
	assert(value1 == nullptr); // Should be evicted

	auto value2 = cache.Get("short2");
	assert(value2 == nullptr); // Should be evicted

	auto value3 = cache.Get("long1");
	assert(value3 != nullptr); // Should still be there

	std::cout << "TimeLRU time-based eviction test completed!" << std::endl;
	cache.Stop();
	thread.join();
}

void testTimeLRUConstMethods() {
	std::cout << "\n=== Testing TimeLRU Const Methods ===" << std::endl;
	TimeLRUCache<std::string, std::string, 10> cache;
	auto thread = StartTimer(cache);
	cache.Put("test", "value", 5); // 5 seconds timeout

	const auto &constCache = cache;
	assert(constCache.Size() == 1);

	auto result = constCache.Get("test");
	assert(result != nullptr);
	assert(*result == "value");

	auto nonexistent = constCache.Get("nonexistent");
	assert(nonexistent == nullptr);

	std::cout << "TimeLRU const methods test passed!" << std::endl;
	cache.Stop();
	thread.join();
}

int main() {
	testBasicTimeLRUOperations();
	testTimeLRUTryPut();
	testTimeLRUManualEviction();
	testTimeLRUThreadSafety();
	testTimeLRUTimeBasedEviction();
	testTimeLRUConstMethods();

	std::cout << "\n=== All TimeLRU tests completed ===" << std::endl;
	return 0;
}

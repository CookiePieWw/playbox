#include "lrucache.h"
#include <cassert>
#include <iostream>
#include <string>

void testBasicOperations() {
	std::cout << "=== Testing Basic Operations ===" << std::endl;
	LRUCache<std::string, std::string> cache;

	// Test Put and Get
	cache.Put("key1", "value1");
	cache.Put("key2", "value2");
	cache.Put("key3", "value3");

	assert(cache.Size() == 3);

	auto value = cache.Get("key1");
	assert(value != nullptr);
	assert(*value == "value1");

	value = cache.Get("key2");
	assert(value != nullptr);
	assert(*value == "value2");

	value = cache.Get("nonexistent");
	assert(value == nullptr);

	std::cout << "Basic operations test passed!" << std::endl;
}

void testTryPut() {
	std::cout << "\n=== Testing TryPut ===" << std::endl;
	LRUCache<std::string, std::string> cache;

	bool result = cache.TryPut("key1", "value1");
	assert(result == true);

	result = cache.TryPut("key1", "value1_updated");
	assert(result == false);

	auto value = cache.Get("key1");
	assert(value != nullptr);
	assert(*value == "value1"); // Should still be original value

	std::cout << "TryPut test passed!" << std::endl;
}

void testLRUBehavior() {
	std::cout << "\n=== Testing LRU Behavior ===" << std::endl;
	LRUCache<int, std::string> cache;

	// Add items
	cache.Put(1, "one");
	cache.Put(2, "two");
	cache.Put(3, "three");
	assert(cache.Size() == 3);

	// Access item 1 to make it most recently used
	auto value = cache.Get(1);
	assert(value != nullptr);
	assert(*value == "one");

	// Add more items
	cache.Put(4, "four");
	assert(cache.Size() == 4);

	// Evict least recently used (should be key 2)
	cache.Evict();
	assert(cache.Size() == 3);

	// Check that key 2 was evicted (least recently used)
	auto val2 = cache.Get(2);
	assert(val2 == nullptr);

	// Check that other keys remain
	auto val1 = cache.Get(1);
	auto val3 = cache.Get(3);
	auto val4 = cache.Get(4);
	assert(val1 != nullptr && *val1 == "one");
	assert(val3 != nullptr && *val3 == "three");
	assert(val4 != nullptr && *val4 == "four");

	std::cout << "LRU behavior test passed!" << std::endl;
}

void testUpdateExistingKey() {
	std::cout << "\n=== Testing Update Existing Key ===" << std::endl;
	LRUCache<std::string, int> cache;

	cache.Put("counter", 1);
	auto value = cache.Get("counter");
	assert(value != nullptr);
	assert(*value == 1);

	cache.Put("counter", 42);
	value = cache.Get("counter");
	assert(value != nullptr);
	assert(*value == 42);

	assert(cache.Size() == 1);

	std::cout << "Update existing key test passed!" << std::endl;
}

void testEvictEmptyCache() {
	std::cout << "\n=== Testing Evict Empty Cache ===" << std::endl;
	LRUCache<std::string, std::string> cache;

	assert(cache.Size() == 0);
	cache.Evict(); // Should not crash
	assert(cache.Size() == 0);

	std::cout << "Evict empty cache test passed!" << std::endl;
}

void testGetMethods() {
	std::cout << "\n=== Testing Get Methods ===" << std::endl;
	LRUCache<std::string, std::string> cache;
	cache.Put("test", "value");

	// Test non-const Get (returns pointer)
	auto result = cache.Get("test");
	assert(result != nullptr);
	assert(*result == "value");

	auto nonexistent = cache.Get("nonexistent");
	assert(nonexistent == nullptr);

	std::cout << "Get methods test passed!" << std::endl;
}

void testEvictKey() {
	std::cout << "\n=== Testing Evict Key ===" << std::endl;
	LRUCache<std::string, std::string> cache;
	cache.Put("key1", "value1");
	cache.Put("key2", "value2");

	assert(cache.Size() == 2);

	cache.Evict("key1");
	assert(cache.Size() == 1);
	auto value = cache.Get("key1");
	assert(value == nullptr); // key1 should be evicted

	value = cache.Get("key2");
	assert(value != nullptr && *value == "value2"); // key2 should still exist

	std::cout << "Evict key test passed!" << std::endl;
}

// Uncomment the following lines to run the tests
// int main() {
// 	testBasicOperations();
// 	testTryPut();
// 	testLRUBehavior();
// 	testUpdateExistingKey();
// 	testEvictEmptyCache();
// 	testGetMethods();
//
// 	std::cout << "\n=== All tests completed ===" << std::endl;
// 	return 0;
// }

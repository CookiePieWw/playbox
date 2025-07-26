#pragma once

#include "lrucache.h"
#include "timewheel.h"
#include <memory>
#include <mutex>
#include <vector>

template <typename Key, typename Value, size_t SlotNum>
struct TimeLRUCache
	: std::enable_shared_from_this<TimeLRUCache<Key, Value, SlotNum>> {
	struct TimerTask {
		Key key;
		TimeLRUCache<Key, Value, SlotNum> *cache;
		TimerTask(Key k, TimeLRUCache<Key, Value, SlotNum> *c)
			: key(std::move(k)), cache(c) {}
		void Evict();
	};

	LRUCache<Key, Value> cache;
	TimeWheel<TimerTask, SlotNum, std::vector> timeWheel;
	// TODO: Use fine-grained locking like a thread safe map.
	mutable std::mutex mutex;

	size_t Size() const;

	void Start();
	void Stop();

	// Put a key-value pair into the cache.
	// If the cache already contains the key, update the value.
	// TODO: If key and value are not copy-constructible and assignable?
	void Put(Key key, Value value, size_t interval);

	// Try to put a key-value pair into the cache
	// If the cache already contains the key, return false.
	bool TryPut(Key key, Value value, size_t interval);

	// Get the reference of the value associated with the key.
	// TODO: Replace `const Value *` with `std::optional<const Value &>` with
	// C++26 standard.
	const Value *Get(const Key &key) const;

	// Get the mutable reference of the value associated with the key.
	// TODO: Replace `Value *` with `std::optional<Value &>` with
	// C++26 standard.
	Value *Get(const Key &key);

	// Evict a cache slot. If the cache is empty, do nothing.
	void Evict();

	// Evict specific key from the cache.
	void Evict(const Key &key);
};

template <typename Key, typename Value, size_t SlotNum>
size_t TimeLRUCache<Key, Value, SlotNum>::Size() const {
	std::scoped_lock<std::mutex> lock(mutex);
	return cache.Size();
}

template <typename Key, typename Value, size_t SlotNum>
void TimeLRUCache<Key, Value, SlotNum>::Start() {
	timeWheel.Start();
}

template <typename Key, typename Value, size_t SlotNum>
void TimeLRUCache<Key, Value, SlotNum>::Stop() {
	timeWheel.Stop();
}

template <typename Key, typename Value, size_t SlotNum>
void TimeLRUCache<Key, Value, SlotNum>::Put(Key key, Value value,
											size_t interval) {
	std::scoped_lock<std::mutex> lock(mutex);
	cache.Put(key, value);
	timeWheel.AddTask(TimerTask{std::move(key), this}, interval);
}

template <typename Key, typename Value, size_t SlotNum>
bool TimeLRUCache<Key, Value, SlotNum>::TryPut(Key key, Value value,
											   size_t interval) {
	std::scoped_lock<std::mutex> lock(mutex);
	auto res = cache.TryPut(key, value);
	timeWheel.AddTask(TimerTask{std::move(key), this}, interval);
	return res;
}

template <typename Key, typename Value, size_t SlotNum>
const Value *TimeLRUCache<Key, Value, SlotNum>::Get(const Key &key) const {
	std::scoped_lock<std::mutex> lock(mutex);
	return cache.Get(key);
}

template <typename Key, typename Value, size_t SlotNum>
Value *TimeLRUCache<Key, Value, SlotNum>::Get(const Key &key) {
	std::scoped_lock<std::mutex> lock(mutex);
	return cache.Get(key);
}

template <typename Key, typename Value, size_t SlotNum>
void TimeLRUCache<Key, Value, SlotNum>::Evict() {
	std::scoped_lock<std::mutex> lock(mutex);
	cache.Evict();
}

template <typename Key, typename Value, size_t SlotNum>
void TimeLRUCache<Key, Value, SlotNum>::Evict(const Key &key) {
	std::scoped_lock<std::mutex> lock(mutex);
	cache.Evict(key);
}

template <typename Key, typename Value, size_t SlotNum>
void TimeLRUCache<Key, Value, SlotNum>::TimerTask::Evict() {
	cache->Evict(key);
}

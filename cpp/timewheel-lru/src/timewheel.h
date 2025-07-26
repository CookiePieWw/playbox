#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <unistd.h>

template <typename T, size_t SlotNum, template <typename> typename Container>
struct TimeWheel {
	Container<Container<T>> slots;
	size_t times;
	std::atomic<bool> stop;

	TimeWheel() : slots(SlotNum), times(0), stop(false) {}
	void Start();
	void Stop() { stop.store(true); }
	void AddTask(T task, size_t interval);
};

template <typename T, size_t SlotNum, template <typename> typename Container>
void TimeWheel<T, SlotNum, Container>::Start() {
	while (!stop.load()) {
		sleep(1);
		for (auto &task : slots.at(times % SlotNum)) {
			task.Evict();
		}
		slots[times % SlotNum].clear();
		++times;
	}
}

template <typename T, size_t SlotNum, template <typename> typename Container>
void TimeWheel<T, SlotNum, Container>::AddTask(T task, size_t interval) {
	if (interval > SlotNum)
		return;
	auto slot_index = (times + interval - 1) % SlotNum;
	slots[slot_index].push_back(task);
}

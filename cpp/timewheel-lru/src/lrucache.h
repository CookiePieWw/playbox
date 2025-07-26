#pragma once

#include <map>

template <typename T> struct LinkNode {
	T data;
	LinkNode<T> *prev;
	LinkNode<T> *next;
};

template <typename T> struct LinkList {
	LinkNode<T> *head;
	LinkNode<T> *tail;
	size_t size;

	LinkList();

	void PushBack(T data);
	void PushFront(T data);
	void PushFront(LinkNode<T> *node);
	void Delete(LinkNode<T> *node);
	void PopBack();
	// Delete the node from the list without freeing memory.
	// This is used to spare the node for reuse.
	void Spare(LinkNode<T> *node);

	~LinkList();
};

template <typename Key, typename Value> struct LRUCache {
	using KeyValue = std::pair<Key, Value>;
	LinkList<KeyValue> list;
	std::map<Key, LinkNode<KeyValue> *> map;

	size_t Size() const;

	// Put a key-value pair into the cache.
	// If the cache already contains the key, update the value.
	// TODO: If key and value are not copy-constructible and assignable?
	void Put(Key key, Value value);

	// Try to put a key-value pair into the cache
	// If the cache already contains the key, return false.
	bool TryPut(Key key, Value value);

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

template <typename T> LinkList<T>::LinkList() {
	head = new LinkNode<T>();
	tail = head;
	size = 0;
}

template <typename T> void LinkList<T>::PushBack(T data) {
	LinkNode<T> *node = new LinkNode<T>{T(std::move(data)), nullptr, nullptr};
	tail->next = node;
	node->prev = tail;
	node->next = nullptr;
	tail = node;
	size += 1;
}

template <typename T> void LinkList<T>::PushFront(T data) {
	LinkNode<T> *node = new LinkNode<T>{T(std::move(data)), nullptr, nullptr};
	PushFront(node);
}

template <typename T> void LinkList<T>::PushFront(LinkNode<T> *node) {
	node->next = head->next;
	if (head->next) {
		head->next->prev = node;
	}
	head->next = node;
	node->prev = head;
	if (tail == head) {
		tail = node;
	}
	size += 1;
}

template <typename T> void LinkList<T>::Spare(LinkNode<T> *node) {
	node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	else
		tail = node->prev;
	size -= 1;
}

template <typename T> void LinkList<T>::Delete(LinkNode<T> *node) {
	Spare(node);
	delete node;
}

template <typename T> void LinkList<T>::PopBack() {
	if (size == 0)
		return;
	Delete(tail);
}

template <typename T> LinkList<T>::~LinkList() {
	while (size > 0) {
		PopBack();
	}
	delete head; // Delete the dummy head node
}

template <typename Key, typename Value>
size_t LRUCache<Key, Value>::Size() const {
	return list.size;
}

template <typename Key, typename Value>
void LRUCache<Key, Value>::Put(Key key, Value value) {
	if (map.find(key) != map.end()) {
		auto node = map.at(key);
		node->data = std::make_pair(std::move(key), std::move(value));
	} else {
		list.PushFront(std::make_pair(key, value));
		map.emplace(std::move(key), list.head->next);
	}
}

template <typename Key, typename Value>
bool LRUCache<Key, Value>::TryPut(Key key, Value value) {
	if (map.find(key) != map.end()) {
		return false;
	} else {
		list.PushBack(std::make_pair(key, value));
		map.emplace(std::move(key), list.tail);
		return true;
	}
}

template <typename Key, typename Value>
const Value *LRUCache<Key, Value>::Get(const Key &key) const {
	return const_cast<LRUCache<Key, Value> *>(this)->Get(key);
}

template <typename Key, typename Value>
Value *LRUCache<Key, Value>::Get(const Key &key) {
	auto it = map.find(key);
	if (it == map.end()) {
		return nullptr;
	} else {
		auto node = it->second;
		auto ptr = &node->data;
		list.Spare(node);
		list.PushFront(node);
		return &(ptr->second);
	}
}

template <typename Key, typename Value> void LRUCache<Key, Value>::Evict() {
	if (list.size == 0) {
		return;
	}
	auto node = list.tail;
	map.erase(node->data.first);
	list.PopBack();
}

template <typename Key, typename Value>
void LRUCache<Key, Value>::Evict(const Key &key) {
	auto it = map.find(key);
	if (it != map.end()) {
		auto node = it->second;
		map.erase(it);
		list.Delete(node);
	}
}

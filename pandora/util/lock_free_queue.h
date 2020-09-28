#pragma once
#include "../pandora_declare.h"
#include <memory>
#include <atomic>

PANDORA_NAMESPACE_START

// 只能用于单生产者单消费者的无锁队列
template<typename T>
class lock_free_queue
{
public:
	lock_free_queue() :
		head(new node), tail(head.load())
	{}
	lock_free_queue(const lock_free_queue& other) = delete;
	lock_free_queue& operator=(const lock_free_queue& other) = delete;
	~lock_free_queue()
	{
		while (node* const old_head = head.load())
		{
			head.store(old_head->next);
			delete old_head;
		}
	}
	// pop the head element
	boost::shared_ptr<T> pop()
	{
		node* old_head = pop_head();
		if (!old_head)
		{
			return boost::shared_ptr<T>();
		}
		boost::shared_ptr<T> const res(old_head->data);
		delete old_head;
		return res;
	}

	// push the element
	void push(boost::shared_ptr<T> new_data)
	{
		node* p = new node;
		node* const old_tail = tail.load();
		old_tail->data.swap(new_data);
		old_tail->next = p;
		tail.store(p);
	}

	// if the queue empty
	bool empty()
	{
		node* const old_head = head.load();
		return old_head == tail.load();
	}
private:
	// 节点信息存储
	struct node
	{
		boost::shared_ptr<T> data;
		node* next;
		node() : next(nullptr)
		{}
	};
	std::atomic<node*> head;	// 头节点
	std::atomic<node*> tail;	// 尾节点
	node* pop_head()			// 弹出头节点
	{
		node* const old_head = head.load();
		if (old_head == tail.load())
		{
			return nullptr;
		}
		head.store(old_head->next);
		return old_head;
	}
};

PANDORA_NAMESPACE_END
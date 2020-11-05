#pragma once
#include "../pandora_declare.h"
#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

PANDORA_NAMESPACE_START

// A pool of io_service objects.
class io_service_pool
	: private boost::noncopyable
{
public:
	/// Construct the io_service pool.
	explicit io_service_pool(std::size_t pool_size=0)
		: next_idx_(0)
		,block_(false)
	{
		if(pool_size == 0)
		{
			pool_size = boost::thread::hardware_concurrency();
		}

		for (std::size_t i = 0; i < pool_size; ++i)
		{
			io_srv_ptr io_service(new boost::asio::io_service);
			io_srvs_.push_back(io_service);

			hold_ptr work(new boost::asio::io_service::work(*io_service));
			works_.push_back(work);
		}
	}
	// Destruct the pool object.
	~io_service_pool()
	{
		// Stop all io_service objects in the pool.
		stop();
	}
public:
	/// Run all io_service objects in the pool.
	void run(bool run_with_block)
	{
		if (!threads_.empty()) return;

		block_ = run_with_block;

		// Create a pool of threads to run all of the io_services.
		for (std::size_t i = 0; i < io_srvs_.size(); ++i)
		{
			thread_ptr t(new boost::thread(boost::bind(&boost::asio::io_service::run, io_srvs_[i])));
			threads_.push_back(t);
		}
		if ( block_ )
		{
			block();
		}
	}

	void start()
	{
		run(false);
	}

	/// Stop all io_service objects in the pool.
	void stop()
	{
		if (works_.empty()) {return; }
		{
			boost::mutex::scoped_lock l(mutex_);

			// reset workers 
			for (std::size_t i = 0; i < works_.size(); ++i)
			{
				works_[i].reset();
			}
			works_.clear();

			// stop io service
			for (std::size_t i = 0; i < io_srvs_.size(); ++i)
			{
				io_srvs_[i]->stop();
			}
			//io_srvs_.clear();
		}
	}

	void block()
	{
		// Wait for all threads in the pool to exit.
		for (std::size_t i = 0; i < threads_.size(); ++i)
			threads_[i]->join();

		threads_.clear();
	}

	/// Get an io_service to use.
	boost::asio::io_service& get_io_service()
	{
		// Use a round-robin scheme to choose the next io_service to use.
		boost::asio::io_service& io_service = *io_srvs_[next_idx_];

		{
			boost::mutex::scoped_lock l(mutex_);
			++next_idx_;
			if (next_idx_ == io_srvs_.size())
			{
				next_idx_ = 0;
			}
		}
		return io_service;
	}

private:
	typedef boost::shared_ptr<boost::asio::io_service> io_srv_ptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> hold_ptr;
	typedef boost::shared_ptr<boost::thread> thread_ptr;

	/// The pool of io_services.
	std::vector<io_srv_ptr> io_srvs_;

	/// The work that keeps the io_services running.
	std::vector<hold_ptr> works_;

	// thread pool
	std::vector<thread_ptr> threads_;

	/// The next io_service to use for a connection.
	std::size_t next_idx_;

	// mutex for controlling visit
	boost::mutex mutex_;

private:
	// block the call thread
	bool block_;
};

PANDORA_NAMESPACE_END
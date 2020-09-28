#pragma once
#include "../pandora_declare.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>

PANDORA_NAMESPACE_START

class ThreadBase
{
public:
	ThreadBase() : io_service_(2)
	{
		thread_.reset();
	}
	~ThreadBase()
	{
		if(thread_.get())
		{
			stop();
		}
	}
public:
	virtual void run()
	{
		try
		{
			if(!thread_.get())
			{
				work_.reset(new boost::asio::io_service::work(io_service_));
				thread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)) );
			}
		}
		catch(std::exception &e)
		{
			std::cerr << e.what() << std::endl;
		}
		catch(...)
		{
			std::cerr << "error thread exit" << std::endl;
		}
	}
	void block()
	{
		thread_->join();
	}
	void stop()
	{
		if(work_)
		{	// reset the thread dispatch
			work_.reset();
			io_service_.stop();
			thread_.reset();
		}
	}

	void start()
	{
		run();
	}
public:
	inline boost::asio::io_service &get_io_service()
	{
		return io_service_;
	}
private:
	boost::asio::io_service io_service_;
	typedef boost::scoped_ptr<boost::thread> thread_ptr_type;
	thread_ptr_type thread_;

	typedef boost::scoped_ptr<boost::asio::io_service::work> work_ptr_type;
	/// The work that keeps the io_services running.
	work_ptr_type work_;
};

PANDORA_NAMESPACE_END
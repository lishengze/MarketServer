#pragma once
#include "../pandora_declare.h"
#include "io_service_pool.h"
#include <boost/asio.hpp>

PANDORA_NAMESPACE_START

class ThreadBasePool
{
public:
	ThreadBasePool(io_service_pool &pool):work_pool_(pool)
	{
		io_service_ = &(pool.get_io_service()); 
	}
	virtual ~ThreadBasePool()
	{
		work_pool_.stop();
	}
public:
	inline boost::asio::io_service &get_io_service()
	{
		return *io_service_;
	}
	inline io_service_pool &pool()
	{
		return work_pool_;
	}
private:
	// one of the io service of the service pool
	boost::asio::io_service *io_service_; 
	// io service pool
	io_service_pool &work_pool_;
};

PANDORA_NAMESPACE_END
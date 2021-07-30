#pragma once

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <thread>
#include <future>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <chrono>
#include <mutex>
#include <list>
#include <vector>
#include <queue>
#include <condition_variable>

using namespace std;

#define DECLARE_PTR(X) typedef boost::shared_ptr<X> X##Ptr     /** < define smart ptr > */
#define FORWARD_DECLARE_PTR(X) class X; DECLARE_PTR(X)         /** < forward defile smart ptr > */

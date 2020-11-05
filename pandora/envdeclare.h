#pragma once

#include <thread>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <cstring>
#include <sstream>
#include <set>
#include <mutex>
#include <boost/shared_ptr.hpp>
#include <list>

// #include <boost/shared_ptr.hpp>
// define ThreadPtr as boost's smart pointer, either std::shared_ptr is okay
typedef boost::shared_ptr<std::thread> ThreadPtr;

using std::string;          /** < default using std::string > */
using std::vector;          /** < default using std::vector > */
using std::map;             /** < default using std::map > */
using std::set;             /** < default using std::set > */
using boost::shared_ptr;    /** < default using boost::shared_ptr > */
using std::list;
using std::pair;

// here to define boost smart ptr, cause we do communication with python code
#define DECLARE_PTR(X) typedef boost::shared_ptr<X> X##Ptr     /** < define smart ptr > */
#define FORWARD_DECLARE_PTR(X) class X; DECLARE_PTR(X)         /** < forward defile smart ptr > */

#define UTRADE_INSTALL_PATH "/opt/utrade/console/"              /** < define the utrade default directory > */
#define UTRADE_DATA_FOLDER "/shared/utrade/"                    /** < define the data directory > */
#define UTRADE_LOG_FOLDER UTRADE_DATA_FOLDER "log/"             /** < define the log directory >*/

#define MAX_ORDER_VOLUME    1000000     /** < define the max order volume in single order > **/

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>

#define sleep(n)    Sleep(n)
#endif
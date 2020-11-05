#include "time_util.h"
#include <chrono>

USING_PANDORA_NAMESPACE

// initialize
boost::shared_ptr<NanoTimer> NanoTimer::m_ptr = nullptr;

NanoTimer* NanoTimer::getInstance()
{
    if (m_ptr.get() == nullptr)
    {
        m_ptr = boost::shared_ptr<NanoTimer>(new NanoTimer());
    }
    return m_ptr.get();
}

inline std::chrono::steady_clock::time_point get_time_now()
{
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return std::chrono::steady_clock::time_point(
            std::chrono::steady_clock::duration(
                    std::chrono::seconds(tp.tv_sec) + std::chrono::nanoseconds(tp.tv_nsec)
            )
    );
}

inline long get_socket_diff()
{
    return 0;
}

inline long get_local_diff()
{
    int unix_second_num = std::chrono::seconds(std::time(NULL)).count();            // 系统的当前时间(秒)
    int tick_second_num = std::chrono::duration_cast<std::chrono::seconds>(         // 系统从启动到现在经历的时间(秒)
            get_time_now().time_since_epoch()
    ).count();
    return (unix_second_num - tick_second_num) * NANOSECONDS_PER_SECOND;            // 系统启动之前所经历的时间
}

NanoTimer::NanoTimer()
{
    secDiff = get_local_diff();
    // std::cout << "local diff " << secDiff << std::endl;

    // struct tm* _tm = localtime(&t0);	// 读取现在本地时间 本地int -> 本地tm
	// time_t t1 = mktime(_tm);			// 从本地时间到utc 本地tm ->utc int
	// _tm = gmtime(&t0);				// gmtime        本地int -> utc tm
    time_t t{ time(NULL) };
	struct tm *local_time_ptr = localtime(&t);
	time_t local_time = mktime(local_time_ptr);
	struct tm *utc_time_ptr = gmtime(&t);
	time_t utc_time = mktime(utc_time_ptr);
    timeZone_Diff = difftime(local_time, utc_time);
}

long NanoTimer::getNano() const
{
    long _nano = std::chrono::duration_cast<std::chrono::nanoseconds>(
            get_time_now().time_since_epoch()
    ).count();
    return _nano + secDiff;
}

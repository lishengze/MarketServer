#pragma once

#include "../pandora_declare.h"
#include <sys/time.h>
#include <sstream>
#include <iomanip>

const long MILLISECONDS_PER_SECOND = 1000;
const long MICROSECONDS_PER_MILLISECOND = 1000;
const long NANOSECONDS_PER_MICROSECOND = 1000;

const long MICROSECONDS_PER_SECOND =
        MICROSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
const long NANOSECONDS_PER_MILLISECOND =
        NANOSECONDS_PER_MICROSECOND * MICROSECONDS_PER_MILLISECOND;
const long NANOSECONDS_PER_SECOND =
        NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;

const int SECONDS_PER_MINUTE = 60;
const int MINUTES_PER_HOUR = 60;
const int HOURS_PER_DAY = 24;
const int SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;

const long MILLISECONDS_PER_MINUTE =
        MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE;
const long NANOSECONDS_PER_MINUTE = NANOSECONDS_PER_SECOND * SECONDS_PER_MINUTE;
const long NANOSECONDS_PER_HOUR = NANOSECONDS_PER_SECOND * SECONDS_PER_HOUR;
const long NANOSECONDS_PER_DAY = NANOSECONDS_PER_HOUR * HOURS_PER_DAY;

PANDORA_NAMESPACE_START
/**
 * timer for nanosecond, main class
 */
class NanoTimer;
class NanoTimer {
public:
    /** return current nano time: unix-timestamp * 1e9 + nano-part */
    long getNano() const;
    /** return secDiff */
    inline long getSecDiff() const { return secDiff; }
    /*** set SecDiff */
    inline void setSecDiff(const long& diff) { secDiff = diff; }
    /** singleton */
    static NanoTimer* getInstance();
    // time zone diff
    long timeZoneDiff() { return timeZone_Diff; }

private:
    NanoTimer();
    /** singleton */
    static boost::shared_ptr<NanoTimer> m_ptr;
    /** object to be updated every time called */
    long secDiff;
    // time zone diff
    long timeZone_Diff;
};

/**
 * util function to utilize NanoTimer
 * @return second diff in long for nano time matching
 */
inline long getSecDiff() {
    return NanoTimer::getInstance()->getSecDiff();
}

/**
 * util function to utilize NanoTimer
 * @return none
 * @diff long for nano time matching
 */
inline void setSecDiff(const long& diff) {
    NanoTimer::getInstance()->setSecDiff(diff);
}

/**
 * parse struct tm to nano time
 * @param _tm ctime struct
 * @return nano time in long
 */
inline long ToSecondFromStruct(struct tm _tm) {
    return mktime(&_tm) * NANOSECONDS_PER_SECOND;
}

/**
 * parse string time to nano time
 * @param timeStr string-formatted time
 * @param format eg: %Y%m%d-%H:%M:%S
 * @return nano time in long
 */
inline long ToSecond(const char* timeStr, const char* format) {
    struct tm _tm;
    strptime(timeStr, format, &_tm);
    return ToSecondFromStruct(_tm);
}

/**
 * parse string time to nano time
 * @param timeStr string-formatted time
 * @param format eg: %Y%m%d-%H:%M:%S
 * @return nano time in long
 */
inline long ToNano(const char* timeStr, const char* format) {
    struct tm _tm;
    strptime(timeStr, format, &_tm);
    long nano_time = ToSecondFromStruct(_tm);
    nano_time +=
            NanoTimer::getInstance()->timeZoneDiff() * NANOSECONDS_PER_SECOND;
    std::string src_str{ timeStr };
    size_t pos = src_str.find(".");
    if (pos != std::string::npos) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(9)
           << std::setiosflags(std::ios::left)
           << src_str.substr(pos + 1, src_str.length() - pos - 1);
        nano_time += atoi(ss.str().c_str());
    }
    return nano_time;
}

/**
 * dump long time to string with format
 * @param nano nano time in long
 * @param format eg: %Y%m%d-%H:%M:%S
 * @return string-formatted time
 */
inline std::string ToSecondStr(long nano, const char* format) {
    if (nano <= 0)
        return std::string("NULL");
    nano /= NANOSECONDS_PER_SECOND;
    struct tm* dt;
    char buffer[30];
    dt = gmtime(&nano);
    strftime(buffer, sizeof(buffer), format, dt);
    return std::string(buffer);
}

/**
 * dump long time to struct tm
 * @param nano nano time in long
 * @return ctime struct
 */
inline struct tm ToSecondStruct(long nano) {
    time_t sec_num = nano / NANOSECONDS_PER_SECOND;
    return *gmtime(&sec_num);
}

/**
 * util function to utilize NanoTimer
 * @return current nano time in long (unix-timestamp * 1e9 + nano-part)
 */
inline long NanoTime() {
    std::chrono::high_resolution_clock::time_point curtime = std::chrono::high_resolution_clock().now();
    long orin_nanosecs = std::chrono::duration_cast<std::chrono::nanoseconds>(curtime.time_since_epoch()).count();
    return orin_nanosecs;
//    return NanoTimer::getInstance()->getNano();
}

/*

*/
inline std::string NanoTimeStr() {
    long nano_time = NanoTime();
    string time_now = ToSecondStr(nano_time, "%Y-%m-%d %H:%M:%S");
    time_now += "." + std::to_string(nano_time % NANOSECONDS_PER_SECOND);
    return time_now;
}

/*
 * linux timespec
 */
inline timespec GetTimespec() {
    timespec ts{ 0, 0 };
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
}

PANDORA_NAMESPACE_END
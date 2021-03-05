#ifndef _REPORTER_H_
#define _REPORTER_H_

namespace CProjUtil{

#if defined(WIN32) || defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
#    ifdef REPORTER_DLL
#        ifdef REPORTER_EXPORT
#            define REPORTER_API __declspec(dllexport)
#        else
#            define REPORTER_API __declspec(dllimport)
#        endif
#    else
#        define REPORTER_API
#    endif
#else
#    define REPORTER_API
#endif

    class REPORTER_API ReporterMsg;
    class REPORTER_API ReporterInterface
    {
    public:
        static bool StartMonitor(const char* host, const char* port);
        static bool StopMonitor();
        static bool SendMsg(ReporterMsg* msg);
        static ReporterMsg* CreateMontiorMsg(const char* host, const char* alias, const char* type);
        static bool AddField(ReporterMsg* msg, const char* name, const char* value);
        static bool GetLocalHost(char* buffer, int& len);
        /*
         * Send SMS
         * To be implemented later.
         */
        //bool SendSMS(std::string content, std::string phonenumbers);
    };
}

#endif

#pragma once
#include "../pandora_declare.h"
#include <unistd.h>
#include <cstring>

PANDORA_NAMESPACE_START

#define MAX_PATH 1024
inline std::string get_module_path()
{
    char szWorkPath[MAX_PATH];
    memset(szWorkPath,'\0',MAX_PATH);
    int nRet = readlink("/proc/self/exe", szWorkPath , MAX_PATH);
    if(nRet>MAX_PATH || nRet<0)
    {
        return  "";
    }
    // the path above execute file
    int find_count = 0;
    for(int i=nRet; i>0; i--)
    {
        if(szWorkPath[i]=='/' || szWorkPath[i]=='\\')
        {
            szWorkPath[i]='\0';
            find_count++;
            if (find_count==2)
                break;
        }
    }
    //这就是最终的文件路径，例如  "/usr/var"
    std::string szRet = szWorkPath;
    return szRet;
}

#if defined(WINDOWS) || defined(WIN32)
    #include <direct.h>
    #include <io.h>
#else
    #include <stdarg.h>
    #include <sys/stat.h>
#endif
 
#if defined(WINDOWS) || defined(WIN32)
    #define ACCESS _access
    #define MKDIR(a) _mkdir((a))
#else
    #define ACCESS access
    #define MKDIR(a) mkdir((a),0755)
#endif

inline int CheckPath(const char *pDir, bool include_file_name=false)
{
	int i = 0;
	int iRet;
	int iLen;
	char* pszDir;
 
	if(NULL == pDir)
	{
		return 0;
	}
	pszDir = strdup(pDir);
	iLen = strlen(pszDir);
 
	// 创建中间目录，从第１个字符而不是从第０个字符开始
	for (i = 1; i < iLen;i ++)
	{
		if (pszDir[i] == '\\' || pszDir[i] == '/')
		{ 
			pszDir[i] = '\0';
			//如果不存在,创建
			iRet = ACCESS(pszDir,0);
			if (iRet != 0)
			{
				iRet = MKDIR(pszDir);
				if (iRet != 0)
				{
					return -1;
				} 
			}
			//支持linux,将所有\换成/
			pszDir[i] = '/';
		} 
	}
    // 如果不包括文件名，并且最后字符不是目录字符，则需要创建最后一层目录
    if (!include_file_name)
    {
        if (pszDir[strlen(pszDir)-1] != '/' or pszDir[strlen(pszDir)-1] != '\\')
            iRet = MKDIR(pszDir);
    }
	free(pszDir);
	return iRet;
}

PANDORA_NAMESPACE_END
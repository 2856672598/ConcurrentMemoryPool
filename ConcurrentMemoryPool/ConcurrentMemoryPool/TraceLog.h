
#pragma once
#include <stdio.h>
#include <string>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

//#define __DEBUG__

static std::string GetFileName(const std::string& path)
{
	char ch = '/';

#ifdef _WIN32
	ch = '\\';
#endif

	size_t pos = path.rfind(ch);
	if (pos == std::string::npos)
		return path;
	else
		return path.substr(pos + 1);
}

class TraceLog
{
public:
	static TraceLog* GetInstance()
	{
		return &_sInst;
	}

	// 用于调试追溯的trace log
	void __trace_debug(const char* function,
		const char * filename, int line, const char* format, ...)
	{
		// 读取配置文件
#ifdef __DEBUG__
		std::lock_guard<std::mutex> lock(_smtx);

		// 输出调用函数的信息
		//cout << std::this_thread::get_id() << ":" << &smtx;
		fprintf(_fout, "%d->[%s:%d]%s", GetCurrentThreadId(), GetFileName(filename).c_str(), line, function);

		// 输出用户打的trace信息
		va_list args;
		va_start(args, format);
		vfprintf(_fout, format, args);
		va_end(args);

		fflush(_fout);
#endif
	}

	~TraceLog()
	{
		fclose(_fout);
	}

	FILE* _fout = fopen("alloc.log", "w");
	std::mutex _smtx;

	static TraceLog _sInst;
};

#define __TRACE_DEBUG(...)  \
		TraceLog::GetInstance()->__trace_debug(__FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__)
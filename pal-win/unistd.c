#ifndef _unistd_c_INCLUDED
#define _unistd_c_INCLUDED

#define WIN32_LEAN_AND_MEAN

#include <unistd.h>
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <process.h>
#include <signal.h>
#include <stdint.h>
#include <assert.h>
#include <wchar.h>
#include <psapi.h>

static HANDLE hTimer = NULL;
static HANDLE hTimerQueue = NULL;

void pal_init()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    hTimerQueue = CreateTimerQueue();
    assert(hTimerQueue != NULL);

}

//taken from https://stackoverflow.com/a/26085827/742404
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}

int getrusage(int who, struct rusage* usage)
{
    assert(who == RUSAGE_SELF);

    FILETIME createTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;

    if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime) == -1)
        return -1;

    uint64_t userT64=((uint64_t)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
    uint64_t kernelT64 = ((uint64_t)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;

    usage->ru_utime.tv_sec = userT64 / 10 / 1000 / 1000;
    usage->ru_utime.tv_usec = (userT64 / 10) % (1000*1000);

    usage->ru_stime.tv_sec = kernelT64 / 10 / 1000 / 1000;
    usage->ru_stime.tv_usec = (kernelT64 / 10) % (1000 * 1000);

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        usage->ru_maxrss = pmc.PeakWorkingSetSize / 1024;
        usage->ru_idrss = pmc.WorkingSetSize / 1024;
        usage->ru_ixrss = 0;
    }

    return 0;
}

long sysconf(int name)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    switch (name)
    {
    case _SC_PAGESIZE:
        return si.dwPageSize;
    case _SC_NPROCESSORS_ONLN:
        return si.dwNumberOfProcessors;
    default:
        assert(FALSE);
        return 0;
    }
}

#undef signal

static _crt_signal_t alarmHandler = NULL;

_crt_signal_t pal_signal(int sig, _crt_signal_t func)
{
    if (sig == SIGALRM)
    {
        _crt_signal_t old = alarmHandler;
        alarmHandler = func;
        return old;
    }

    return signal(sig, func);
}

static VOID CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    _crt_signal_t h = alarmHandler;
    if (h)
        h(SIGALRM);
}

unsigned int alarm(unsigned int seconds)
{
    if (hTimer)
    {
        BOOL res = DeleteTimerQueueTimer(hTimerQueue, hTimer, NULL);
        assert(res);
        hTimer = NULL;
    }

    if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)TimerCallback, NULL, (DWORD)seconds * (DWORD)1000, 0, 0))
        assert(0);

    return 0;
}

#endif

/*
MIT License

Copyright (c) 2019 Bharath Paulraj <bharathpaul@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <sys/time.h>
static inline char *getTimestamp (void)
{
    struct timeval tv;
    struct tm *pTmp = NULL;

    gettimeofday(&tv, NULL);

    pTmp = localtime(&tv.tv_sec);
    snprintf(vRouterg.cTimeStamp, 32,
             "[%04d-%02d-%02d][%02d:%02d:%02d.%06d]", pTmp->tm_year + 1900,
             pTmp->tm_mon + 1, pTmp->tm_mday, pTmp->tm_hour,
             pTmp->tm_min, pTmp->tm_sec, (int)tv.tv_usec);

    return vRouterg.cTimeStamp;
}

#define LOG_CRIT(fmt, arg ...) RTE_LOG(CRIT, VROUTER, "%s ", getTimestamp ());  \
                                   RTE_LOG(CRIT, VROUTER, " <crit> " fmt "\n", ## arg);

#define LOG_ERR(fmt, arg ...) RTE_LOG(ERR, VROUTER, "%s ", getTimestamp ());  \
                                   RTE_LOG(ERR, VROUTER, " <error> " fmt "\n", ## arg);

#define LOG_WARN(fmt, arg ...) RTE_LOG(WARNING, VROUTER, "%s ", getTimestamp ());  \
                                   RTE_LOG(WARNING, VROUTER, " <warn> " fmt "\n", ## arg);

#define LOG_DEBUG(fmt, arg ...) RTE_LOG(DEBUG, VROUTER, "%s ", getTimestamp ());  \
                                   RTE_LOG(DEBUG, VROUTER, " <debug> " fmt "\n", ## arg);


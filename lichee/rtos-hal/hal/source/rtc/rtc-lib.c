/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <rtc/rtc.h>

static const uint8_t days_per_month[] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

int is_leap_year(uint16_t year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

uint8_t get_days_in_month(uint8_t month, uint16_t year)
{
    if (month == 1 && is_leap_year(year)) {
        return 29;
    }

    return days_per_month[month - 1];
}

int rtc_is_time_valid(struct rtc_time *tm)
{
    if (tm->tm_year < 70
        || tm->tm_mon >= 12
        || tm->tm_mday < 1
        || tm->tm_mday > get_days_in_month(tm->tm_mon, tm->tm_year + 1900)
        || tm->tm_hour >= 24
        || tm->tm_min >= 60
        || tm->tm_sec >= 60)
    {
        return -1;
    }

    return 0;
}

time64_t calculate_timestamp(uint16_t year, uint8_t month, uint8_t day,
                                uint8_t hour, uint8_t minute, uint8_t second)
{
    int64_t seconds = 0;

    for (uint16_t y = 1970; y < year; ++y) {
        seconds += 365 + is_leap_year(y);
    }

    for (uint8_t m = 1; m < month; ++m) {
        seconds += get_days_in_month(m, year);
    }
    seconds += day - 1;
    seconds *= 86400;
    seconds += hour * 3600 + minute * 60 + second;

    return seconds;
}

time64_t convert_tm_to_timestamp(struct rtc_time *tm)
{
    return calculate_timestamp(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                                tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void convert_timestamp_to_tm(int64_t time, struct rtc_time *tm)
{
    int64_t days, secs;
    days = time / 86400;
    secs = time % 86400;

    uint16_t year = 1970;
    while (days >= 365 + is_leap_year(year)) {
        days -= 365 + is_leap_year(year++);
    }

    uint8_t month = 1;
    while (days >= get_days_in_month(month, year)) {
        days -= get_days_in_month(month++, year);
    }

    tm->tm_year = year - 1900;
    tm->tm_mon = month - 1;
    tm->tm_mday = days + 1;
    tm->tm_hour = secs / 3600;
    tm->tm_min = (secs % 3600) / 60;
    tm->tm_sec = (secs % 3600) % 60;
    tm->tm_wday = (4 + days) % 7;
    tm->tm_yday = days + 1;
    tm->tm_isdst = 0;
}

time64_t rtc_tm_sub(struct rtc_time *lhs, struct rtc_time *rhs)
{
    return convert_tm_to_timestamp(lhs) - convert_tm_to_timestamp(rhs);
}
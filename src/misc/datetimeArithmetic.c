#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#undef BOOL
#undef BOOLEAN
#undef uchar
#undef uint8
#undef int8
#undef uint16
#undef int16
#undef uint32
#undef int32
#undef uint64
#undef int64
#undef float32
#undef float64

#ifndef BOOL
#define BOOL    int
#endif

#ifndef BOOLEAN
#define BOOLEAN unsigned char
#endif

#ifndef uchar
#define uchar   unsigned char
#endif

#ifndef uint8
#define uint8    unsigned char
#endif

#ifndef int8
#define int8    signed char
#endif

#ifndef uint16
#define uint16    unsigned short
#endif

#ifndef int16
#define int16    short
#endif

#ifndef uint32
#define uint32    unsigned int
#endif

#ifndef int32
#define int32    int
#endif

#ifndef uint64
#define uint64    unsigned long long
#endif

#ifndef int64
#define int64    long long
#endif

#ifndef float32
#define float32    float
#endif

#ifndef float64
#define float64    double
#endif

/** @brief 时间间隔单位   */
typedef enum tag_Time_Interval_Span
{
    TI_SEC = 0,
    TI_MIN = 1,
    TI_HOUR = 2,
    TI_DAY = 3,
    TI_MON = 4,
    TI_YEAR = 5
} OOP_TI_SPAN_E;

/** @brief 时间间隔     */
typedef struct tag_Time_Interval
{
    uint8 unit; /**< 间隔单位(见OOP_TI_SPAN_E)   */
    uint16 value; /**< 间隔值                        */
} OOP_TI_T;

/** @brief 长时间格式 日期-时间  */
typedef struct tag_DATETIME
{
    uint16 year; /**< 年          */
    uint8 month; /**< 月          */
    uint8 mday; /**< 日          */
    uint8 wday; /**< 周          */
    uint8 hour; /**< 时          */
    uint8 minute; /**< 分          */
    uint8 second; /**< 秒          */
    uint16 msec; /**< 毫秒     */
} OOP_DATETIME_T;

uint8 is_leap_year(uint16 year)
{
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        return 1;
    }

    return 0;
}

uint8 calculate_weekday(uint16 year, uint8 month, uint8 day)
{
    // Use wider accumulators to avoid truncation and implement Zeller's congruence.
    if (month < 3)
    {
        month += 12;
        year--;
    }

    int k = year % 100;
    int j = year / 100;
    int f = day + (13 * (month + 1)) / 5 + k + (k / 4) + (j / 4) + (5 * j);
    int h = f % 7; // Zeller's result: 0=Saturday,1=Sunday,2=Monday,...

    // Return 1..7 with 1 = Monday (comment expectation). Mapping: Monday->1 ... Sunday->7
    return (uint8)(((h + 5) % 7) + 1);
}

uint8 get_days_in_month(uint16 year, uint8 month)
{
    switch (month)
    {
        case 1:
            return 31;
        case 2:
            return is_leap_year(year) ? 29 : 28;
        case 3:
            return 31;
        case 4:
            return 30;
        case 5:
            return 31;
        case 6:
            return 30;
        case 7:
            return 31;
        case 8:
            return 31;
        case 9:
            return 30;
        case 10:
            return 31;
        case 11:
            return 30;
        case 12:
            return 31;
        default:
            return 0; // 无效月份
    }
}

void addDateTimeByTi(OOP_DATETIME_T *start, OOP_TI_T *interval, OOP_DATETIME_T *result, uint8 sign)
{
    (void)sign; // parameter intentionally unused

    if (start == NULL || interval == NULL || result == NULL)
    {
        return;
    }

    memcpy(result, start, sizeof(OOP_DATETIME_T));

    switch (interval->unit)
    {
        case TI_SEC:
        {
            int64_t total_seconds = (int64_t)result->second + (int64_t)interval->value;
            int64_t carry_minutes = total_seconds / 60;
            result->second = (uint8)(total_seconds % 60);

            if (carry_minutes > 0)
            {
                int64_t total_minutes = (int64_t)result->minute + carry_minutes;
                int64_t carry_hours = total_minutes / 60;
                result->minute = (uint8)(total_minutes % 60);

                if (carry_hours > 0)
                {
                    int64_t total_hours = (int64_t)result->hour + carry_hours;
                    int64_t carry_days = total_hours / 24;
                    result->hour = (uint8)(total_hours % 24);

                    if (carry_days > 0)
                    {
                        OOP_TI_T day_interval = { TI_DAY, (uint16)carry_days };
                        addDateTimeByTi(result, &day_interval, result, 1);
                    }
                }
            }
            break;
        }

        case TI_MIN:
        {
            int64_t total_minutes = (int64_t)result->minute + (int64_t)interval->value;
            int64_t carry_hours = total_minutes / 60;
            result->minute = (uint8)(total_minutes % 60);

            if (carry_hours > 0)
            {
                int64_t total_hours = (int64_t)result->hour + carry_hours;
                int64_t carry_days = total_hours / 24;
                result->hour = (uint8)(total_hours % 24);

                if (carry_days > 0)
                {
                    OOP_TI_T day_interval = { TI_DAY, (uint16)carry_days };
                    addDateTimeByTi(result, &day_interval, result, 1);
                }
            }
            break;
        }

        case TI_HOUR:
        {
            int64_t total_hours = (int64_t)result->hour + (int64_t)interval->value;
            int64_t carry_days = total_hours / 24;
            result->hour = (uint8)(total_hours % 24);

            if (carry_days > 0)
            {
                OOP_TI_T day_interval = { TI_DAY, (uint16)carry_days };
                addDateTimeByTi(result, &day_interval, result, 1);
            }
            break;
        }

        case TI_DAY:
        {
            uint16 remaining_days = interval->value;

            while (remaining_days > 0)
            {
                uint8 days_in_month = get_days_in_month(result->year, result->month);
                /* days left after current day until end of month */
                uint8 days_left = days_in_month - result->mday;

                if (remaining_days <= days_left)
                {
                    result->mday += (uint8)remaining_days;
                    remaining_days = 0;
                }
                else
                {
                    /* consume the rest of this month and move to the first day of next month */
                    remaining_days -= (uint16)(days_left + 1);
                    result->mday = 1;

                    if (++result->month > 12)
                    {
                        result->month = 1;
                        result->year++;
                    }
                }
            }

            result->wday = calculate_weekday(result->year, result->month, result->mday);
            break;
        }

        case TI_MON:
        {
            uint16 total_months = result->month + interval->value;

            result->year += (total_months - 1) / 12;
            result->month = (total_months - 1) % 12 + 1;

            uint8 days_in_month = get_days_in_month(result->year, result->month);
            if (result->mday > days_in_month)
            {
                result->mday = days_in_month;
            }

            result->wday = calculate_weekday(result->year, result->month, result->mday);
            break;
        }

        case TI_YEAR:
        {
            result->year += interval->value;

            if (result->month == 2 && result->mday == 29)
            {
                if (!is_leap_year(result->year))
                {
                    result->mday = 28;
                }
            }

            result->wday = calculate_weekday(result->year, result->month, result->mday);
            break;
        }

        default:
            break;
    }
}

int compareOOPDatetime(OOP_DATETIME_T *a, OOP_DATETIME_T *b)
{
    // 检查输入合法性（避免空指针）
    if (a == NULL || b == NULL)
    {
        return 0;  // 或根据需求处理错误
    }

    // 1. 比较年
    if (a->year != b->year)
    {
        return (a->year < b->year) ? -1 : 1;
    }

    // 2. 年相等，比较月
    if (a->month != b->month)
    {
        return (a->month < b->month) ? -1 : 1;
    }

    // 3. 月相等，比较日
    if (a->mday != b->mday)
    {
        return (a->mday < b->mday) ? -1 : 1;
    }

    // 4. 日相等，比较时
    if (a->hour != b->hour)
    {
        return (a->hour < b->hour) ? -1 : 1;
    }

    // 5. 时相等，比较分
    if (a->minute != b->minute)
    {
        return (a->minute < b->minute) ? -1 : 1;
    }

    // 6. 分相等，比较秒
    if (a->second != b->second)
    {
        return (a->second < b->second) ? -1 : 1;
    }

    // 7. 秒相等，比较毫秒
    if (a->msec != b->msec)
    {
        return (a->msec < b->msec) ? -1 : 1;
    }

    // 所有字段都相等
    return 0;
}

void calcNextRunTime(OOP_DATETIME_T *start, OOP_TI_T *interval, OOP_DATETIME_T *now, OOP_DATETIME_T *nextRunTime)
{
    if (start == NULL || interval == NULL || now == NULL || nextRunTime == NULL)
    {
        return;
    }

    // If start is after now, the next run is start itself.
    if (compareOOPDatetime(start, now) > 0)
    {
        memcpy(nextRunTime, start, sizeof(OOP_DATETIME_T));
        return;
    }

    // Guard against zero interval which would otherwise loop forever.
    if (interval->value == 0)
    {
        // No progression; return start if it's in the future, otherwise return now.
        if (compareOOPDatetime(start, now) > 0)
        {
            memcpy(nextRunTime, start, sizeof(OOP_DATETIME_T));
        }
        else
        {
            memcpy(nextRunTime, now, sizeof(OOP_DATETIME_T));
        }
        return;
    }

    uint64 count = 1;
    OOP_TI_T ti = { 0 };
    ti.unit = interval->unit;
    OOP_DATETIME_T temp = { 0 };

    // Find the smallest n such that start + n*interval > now
    while (1)
    {
        ti.value = (uint16)(interval->value * count);
        addDateTimeByTi(start, &ti, &temp, 1);
        if (compareOOPDatetime(&temp, now) > 0)
        {
            break;
        }
        count++;

        // safety: avoid infinite loop on overflow
        if (count == 0)
        {
            memcpy(nextRunTime, &temp, sizeof(OOP_DATETIME_T));
            return;
        }
    }

    memcpy(nextRunTime, &temp, sizeof(OOP_DATETIME_T));
}
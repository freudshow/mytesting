#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Include the implementation directly so we get the types and functions.
#include "datetimeArithmetic.c"

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT(expr, msg) do { \
    tests_run++; \
    if (!(expr)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } else { \
        printf("ok: %s\n", msg); \
    } \
} while (0)

int equalDatetime(const OOP_DATETIME_T *a, const OOP_DATETIME_T *b)
{
    return a->year == b->year && a->month == b->month && a->mday == b->mday &&
           a->hour == b->hour && a->minute == b->minute && a->second == b->second &&
           a->msec == b->msec;
}

void test_is_leap_year(void)
{
    ASSERT(is_leap_year(2000) == 1, "2000 is leap year");
    ASSERT(is_leap_year(1900) == 0, "1900 is not leap year");
    ASSERT(is_leap_year(2004) == 1, "2004 is leap year");
    ASSERT(is_leap_year(2001) == 0, "2001 is not leap year");
}

void test_calculate_weekday(void)
{
    // Known dates: 2000-01-01 is Saturday -> expect 6 (1=Mon ... 7=Sun)
    ASSERT(calculate_weekday(2000, 1, 1) == 6, "2000-01-01 is Saturday (6)");
    // 2025-12-15 is Monday -> expect 1
    ASSERT(calculate_weekday(2025, 12, 15) == 1, "2025-12-15 is Monday (1)");
    // 2023-01-01 is Sunday -> expect 7
    ASSERT(calculate_weekday(2023, 1, 1) == 7, "2023-01-01 is Sunday (7)");
}

void test_get_days_in_month(void)
{
    ASSERT(get_days_in_month(2021, 2) == 28, "Feb 2021 has 28 days");
    ASSERT(get_days_in_month(2024, 2) == 29, "Feb 2024 has 29 days");
    ASSERT(get_days_in_month(2021, 4) == 30, "Apr has 30 days");
    ASSERT(get_days_in_month(2021, 1) == 31, "Jan has 31 days");
}

void test_addDateTimeByTi_seconds(void)
{
    OOP_DATETIME_T start = {2025, 12, 31, 0, 23, 59, 50, 0};
    OOP_DATETIME_T res;
    OOP_TI_T it = {TI_SEC, 20};

    addDateTimeByTi(&start, &it, &res, 1);
    OOP_DATETIME_T expect = {2026, 1, 1, 0, 0, 0, 10, 0};
    ASSERT(equalDatetime(&res, &expect), "Add 20 seconds across midnight");
}

void test_addDateTimeByTi_minutes_hours_days(void)
{
    OOP_DATETIME_T start = {2025, 12, 31, 0, 23, 30, 0, 0};
    OOP_DATETIME_T res;
    OOP_TI_T it;

    // add 90 minutes -> +1h30 => next day
    it.unit = TI_MIN; it.value = 90;
    addDateTimeByTi(&start, &it, &res, 1);
    OOP_DATETIME_T expect1 = {2026, 1, 1, 0, 1, 0, 0, 0}; // 23:30 + 90min = 01:00 next day
    ASSERT(equalDatetime(&res, &expect1), "Add 90 minutes across day boundary");

    // add 48 hours -> +2 days
    start.year = 2025; start.month = 12; start.mday = 30; start.hour = 0; start.minute = 0; start.second = 0; start.msec = 0;
    it.unit = TI_HOUR; it.value = 48;
    addDateTimeByTi(&start, &it, &res, 1);
    OOP_DATETIME_T expect2 = {2026, 1, 1, 0, 0, 0, 0, 0};
    ASSERT(equalDatetime(&res, &expect2), "Add 48 hours -> +2 days across year boundary");

    // add 1 day to Jan 31 -> Feb 1
    start.year = 2021; start.month = 1; start.mday = 31; start.hour = 0; start.minute = 0; start.second = 0; start.msec = 0;
    it.unit = TI_DAY; it.value = 1;
    addDateTimeByTi(&start, &it, &res, 1);
    OOP_DATETIME_T expect3 = {2021, 2, 1, 0, 0, 0, 0, 0};
    ASSERT(equalDatetime(&res, &expect3), "Add 1 day from Jan31 -> Feb1");
}

void test_addDateTimeByTi_month_year(void)
{
    OOP_DATETIME_T start, res;
    OOP_TI_T it;

    // Jan 31 2021 + 1 month -> Feb 28 2021
    start.year = 2021; start.month = 1; start.mday = 31; start.hour = 0; start.minute = 0; start.second = 0; start.msec = 0;
    it.unit = TI_MON; it.value = 1;
    addDateTimeByTi(&start, &it, &res, 1);
    OOP_DATETIME_T expect1 = {2021, 2, 28, 0, 0, 0, 0, 0};
    ASSERT(equalDatetime(&res, &expect1), "Jan31+1month->Feb28 (non-leap)");

    // Feb 29 2020 + 1 year -> Feb 28 2021
    start.year = 2020; start.month = 2; start.mday = 29; start.hour = 0; start.minute = 0; start.second = 0; start.msec = 0;
    it.unit = TI_YEAR; it.value = 1;
    addDateTimeByTi(&start, &it, &res, 1);
    OOP_DATETIME_T expect2 = {2021, 2, 28, 0, 0, 0, 0, 0};
    ASSERT(equalDatetime(&res, &expect2), "Feb29+1year->Feb28 (non-leap target)");
}

void test_compareOOPDatetime(void)
{
    OOP_DATETIME_T a = {2025,12,1,0,0,0,0,0};
    OOP_DATETIME_T b = {2025,12,1,0,0,0,0,0};
    OOP_DATETIME_T c = {2025,12,2,0,0,0,0,0};

    ASSERT(compareOOPDatetime(&a, &b) == 0, "compare equal datetimes");
    ASSERT(compareOOPDatetime(&a, &c) < 0, "a < c");
    ASSERT(compareOOPDatetime(&c, &a) > 0, "c > a");
}

void test_calcNextRunTime(void)
{
    OOP_DATETIME_T start = {2025,12,1,0,0,0,0,0};
    OOP_TI_T it = {TI_DAY, 1};
    OOP_DATETIME_T now = {2025,12,3,0,1,0,0,0};
    OOP_DATETIME_T next;

    calcNextRunTime(&start, &it, &now, &next);

    OOP_DATETIME_T expect = {2025,12,4,0,0,0,0,0};
    ASSERT(equalDatetime(&next, &expect), "calcNextRunTime daily interval produces expected next run");

    // If interval->value == 0, should return now or start per implementation
    it.unit = TI_DAY; it.value = 0;
    calcNextRunTime(&start, &it, &now, &next);
    ASSERT(equalDatetime(&next, &now), "calcNextRunTime with zero interval returns now when start <= now");
}

int main(void)
{
    printf("Running datetimeArithmetic tests...\n");

    test_is_leap_year();
    test_calculate_weekday();
    test_get_days_in_month();
    test_addDateTimeByTi_seconds();
    test_addDateTimeByTi_minutes_hours_days();
    test_addDateTimeByTi_month_year();
    test_compareOOPDatetime();
    test_calcNextRunTime();

    printf("\nTests run: %d, failed: %d\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}

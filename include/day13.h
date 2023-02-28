#ifndef __DAY13_H
#define __DAY13_H

#include <sys/types.h>
#include <sys/time.h>
#ifdef linux
#include <time.h>
#endif

#include "error13.h"

#ifdef __WIN32
#define BASEYEAR    1900
#else
#define BASEYEAR    1970
#endif

#define D13_ITEMS		6 //year; month; day; hour; minute; second
#define D13_ITEMS_YEAR	0
#define D13_ITEMS_MON	1
#define D13_ITEMS_DAY	2
#define D13_ITEMS_HOUR	3
#define D13_ITEMS_MIN	4
#define D13_ITEMS_SEC	5
#define	D13S_ITEMS_YEAR_MULTI	(10^10)
#define	D13S_ITEMS_MON_MULTI	(10^8)
#define	D13S_ITEMS_DAY_MULTI	(10^6)
#define	D13S_ITEMS_HOUR_MULTI	(10^4)
#define	D13S_ITEMS_MIN_MULTI	(10^2)
#define	D13S_ITEMS_SEC_MULTI	(10^0)
#define D13S_ITEMS_YEAR_SHIFT	20
#define D13S_ITEMS_MON_SHIFT    16
#define D13S_ITEMS_DAY_SHIFT	12
#define D13S_ITEMS_HOUR_SHIFT	8
#define D13S_ITEMS_MIN_SHIFT	4
#define D13S_ITEMS_SEC_SHIFT	0
#define D13S_ITEMS_YEAR_MASK	(0x00FFFF0000000000)
#define D13S_ITEMS_MON_MASK		(0x000000FF00000000)
#define D13S_ITEMS_DAY_MASK		(0x00000000FF000000)
#define D13S_ITEMS_HOUR_MASK	(0x0000000000FF0000)
#define D13S_ITEMS_MIN_MASK		(0x000000000000FF00)
#define D13S_ITEMS_SEC_MASK		(0x00000000000000FF)


#define MAXTIME     20  //yyyy-mm-dd-hh-mm-ss

#define d13_datesep "/.-"

typedef uint64_t d13s_time_t;

#ifdef __cplusplus
	extern "C" {
#endif

/*		--	serialize-able time functions	-- */
error13_t d13s_clock(d13s_time_t* t);
error13_t d13s_get_gtime(d13s_time_t t, int gtime[D13_ITEMS]);
error13_t d13s_get_jtime(d13s_time_t t, int jtime[D13_ITEMS]);

//date-conversion
error13_t d13_g2j(int g_y, int g_m, int g_d, int jdate[D13_ITEMS]);
error13_t d13_j2g(int j_y, int j_m, int j_d, int gdate[D13_ITEMS]);
error13_t d13_today(int date[D13_ITEMS]);
error13_t d13_clock(int* t13_time);
error13_t d13_now(char t13[MAXTIME]);
error13_t d13_time13(time_t* t, char t13[MAXTIME]);
error13_t d13_resolve_date(char* date, int d[D13_ITEMS]);
error13_t d13_fix_jdate(char* src, char date[MAXTIME]);

//gregorian
error13_t d13_time13g(time_t* t, char t13[MAXTIME]);
error13_t d13_todayg(int date[D13_ITEMS]);
error13_t d13_nowg(char t13[MAXTIME]);

uint32_t d13_jdayno(int j_y, int j_m, int j_d);
error13_t d13_jdayno2jdate(unsigned long j_day_no, int jdate[3]);

/*
 * date compare rules:
 * 0 means any...
 * i.e. 0000/??/?? means a specific day in month in all years
 * or ????/00/?? means a specific day in in all months of a specific year.
*/
int d13_cmp_time13(int t13_1[D13_ITEMS], int t13_2[D13_ITEMS]);

#ifdef __cplusplus
	}
#endif

#else

#endif

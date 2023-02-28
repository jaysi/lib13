#include "include/day13.h"
#include "include/str13.h"

#include <stdio.h>
#include <stdlib.h>

#undef NDEBUG
#include "include/debug13.h"
#define _deb_serial _NullMsg

#define __true 1
#define __false 0

#undef TEST
//#define TEST

uint32_t d13_gdayno(int g_y, int g_m, int g_d){
    int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    //int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    int i;
    unsigned long gy, gm, gd, g_day_no;
    //int jy, jm, jd, j_day_no;
    //int j_np;

    gy = g_y-1600;
    gm = g_m-1;
    gd = g_d-1;

    g_day_no = 365*gy+((gy+3)/4)-((gy+99)/100)+((gy+399)/400);

    for (i=0; i < gm; ++i) g_day_no += g_days_in_month[i];

    if (gm>1 && ((gy%4==0 && gy%100!=0) || (gy%400==0))) g_day_no++; /* leap and after Feb */

    g_day_no += gd;

    return g_day_no;
}

uint32_t d13_jdayno(int j_y, int j_m, int j_d){
    //int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    int i;
    //int gy, gm, gd, g_day_no;
    unsigned long jy, jm, jd, j_day_no;
    //int leap;

    if(j_y <= 0 || j_m <= 0 || j_d <= 0) return 0UL;

    jy = j_y-979;
    jm = j_m-1;
    jd = j_d-1;

    j_day_no = 365*jy + (jy/ 33)*8 + ((jy%33+3)/ 4);
    for (i=0; i < jm; ++i) j_day_no += j_days_in_month[i];

    j_day_no += jd;

    return j_day_no;
}

error13_t d13_jdayno2gdate(unsigned long j_day_no, int gdate[3]){

    int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    //int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    int i;
    unsigned long gy, gm, gd, g_day_no;
    //int jy, jm, jd, j_day_no;
    int leap;

    g_day_no = j_day_no+79;

    gy = 1600 + 400*(g_day_no/ 146097); /* 146097 = 365*400 + 400/4 - 400/100 + 400/400 */

    g_day_no = g_day_no % 146097;
    leap = __true;

    if (g_day_no >= 36525) { /* 36525 = 365*100 + 100/4 */
        g_day_no--;
        gy += 100*(g_day_no/ 36524); /* 36524 = 365*100 + 100/4 - 100/100 */
        g_day_no = g_day_no % 36524;
        if (g_day_no >= 365) g_day_no++;
        else leap = __false;
    }

    gy += 4*(g_day_no/ 1461); /* 1461 = 365*4 + 4/4 */ g_day_no %= 1461;
    if (g_day_no >= 366) {
        leap = __false;
        g_day_no--;
        gy += (g_day_no/ 365);
        g_day_no = g_day_no % 365;
    }
    for (i = 0; g_day_no >= g_days_in_month[i] + (i == 1 && leap); i++) g_day_no -= g_days_in_month[i] + (i == 1 && leap);

    gm = i+1;
    gd = g_day_no+1;

    gdate[0] = gy;
    gdate[1] = gm;
    gdate[2] = gd;

    return E13_OK;
}

error13_t d13_gdayno2jdate(unsigned long g_day_no, int jdate[3]){

    //int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    int i;
    //int gy, gm, gd, g_day_no;
    unsigned long jy, jm, jd, j_day_no;
    int j_np;

    j_day_no = g_day_no-79;
    j_np = (j_day_no/ 12053); /* 12053 = 365*33 + 32/4 */

    j_day_no = j_day_no % 12053;
    jy = 979+33*j_np+4*(j_day_no/1461); /* 1461 = 365*4 + 4/4 */

    j_day_no %= 1461;

    if(j_day_no >= 366) {
        jy += ((j_day_no-1)/ 365);
        j_day_no = (j_day_no-1)%365;
    }

    for(i = 0; i < 11 && j_day_no >= j_days_in_month[i]; ++i)
        j_day_no -= j_days_in_month[i];

    jm = i+1;
    jd = j_day_no+1;

    jdate[0] = jy;
    jdate[1] = jm;
    jdate[2] = jd;

    return E13_OK;
}

error13_t d13_jdayno2jdate(unsigned long j_day_no, int jdate[3]){
    /*
     * jd1->j1, j1->gd1, gd1->jd1;
    */

    int gdate[3];
    d13_jdayno2gdate(j_day_no, gdate);
    d13_g2j(gdate[0], gdate[1], gdate[2], jdate);
    return E13_OK;
}


error13_t d13_g2j(int g_y, int g_m, int g_d, int jdate[3]){

    int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    int i;
    int gy, gm, gd, g_day_no;
    int jy, jm, jd, j_day_no;
    int j_np;

    gy = g_y-1600;
    gm = g_m-1;
    gd = g_d-1;

    g_day_no = 365*gy+((gy+3)/4)-((gy+99)/100)+((gy+399)/400);

    for (i=0; i < gm; ++i) g_day_no += g_days_in_month[i];

    if (gm>1 && ((gy%4==0 && gy%100!=0) || (gy%400==0))) g_day_no++; /* leap and after Feb */

    g_day_no += gd;
    j_day_no = g_day_no-79;
    j_np = (j_day_no/ 12053); /* 12053 = 365*33 + 32/4 */

    j_day_no = j_day_no % 12053;
    jy = 979+33*j_np+4*(j_day_no/1461); /* 1461 = 365*4 + 4/4 */

    j_day_no %= 1461;

    if(j_day_no >= 366) {
        jy += ((j_day_no-1)/ 365);
        j_day_no = (j_day_no-1)%365;
    }

    for(i = 0; i < 11 && j_day_no >= j_days_in_month[i]; ++i)
        j_day_no -= j_days_in_month[i];

    jm = i+1;
    jd = j_day_no+1;

    jdate[0] = jy;
    jdate[1] = jm;
    jdate[2] = jd;

    return E13_OK;
}

error13_t d13_j2g(int j_y, int j_m, int j_d, int gdate[3]) {

    int g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int j_days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};
    int i;
    int gy, gm, gd, g_day_no;
    int jy, jm, jd, j_day_no;
    int leap;

    jy = j_y-979;
    jm = j_m-1;
    jd = j_d-1;

    j_day_no = 365*jy + (jy/ 33)*8 + ((jy%33+3)/ 4);
    for (i=0; i < jm; ++i) j_day_no += j_days_in_month[i];

    j_day_no += jd;
    g_day_no = j_day_no+79;

    gy = 1600 + 400*(g_day_no/ 146097); /* 146097 = 365*400 + 400/4 - 400/100 + 400/400 */

    g_day_no = g_day_no % 146097;
    leap = __true;

    if (g_day_no >= 36525) { /* 36525 = 365*100 + 100/4 */
        g_day_no--;
        gy += 100*(g_day_no/ 36524); /* 36524 = 365*100 + 100/4 - 100/100 */
        g_day_no = g_day_no % 36524;
        if (g_day_no >= 365) g_day_no++;
        else leap = __false;
    }

    gy += 4*(g_day_no/ 1461); /* 1461 = 365*4 + 4/4 */ g_day_no %= 1461;
    if (g_day_no >= 366) {
        leap = __false;
        g_day_no--;
        gy += (g_day_no/ 365);
        g_day_no = g_day_no % 365;
    }
    for (i = 0; g_day_no >= g_days_in_month[i] + (i == 1 && leap); i++) g_day_no -= g_days_in_month[i] + (i == 1 && leap);

    gm = i+1;
    gd = g_day_no+1;

    gdate[0] = gy;
    gdate[1] = gm;
    gdate[2] = gd;

    return E13_OK;

}

static inline error13_t _d13_get_proper_sys_time(time_t* t, int it13[D13_ITEMS]){
    struct tm* tp;
    char tmp[10];
    tp = localtime(t);
    if(!tp) return e13_error(E13_SYSE);

    strftime(tmp, 10, "%Y", tp);
    it13[D13_ITEMS_YEAR] = atoi(tmp);
    strftime(tmp, 10, "%m", tp);
    it13[D13_ITEMS_MON] = atoi(tmp);
    strftime(tmp, 10, "%d", tp);
    it13[D13_ITEMS_DAY] = atoi(tmp);

    it13[D13_ITEMS_HOUR] = tp->tm_hour;
    it13[D13_ITEMS_MIN] = tp->tm_min;
    it13[D13_ITEMS_SEC] = tp->tm_sec;

    return E13_OK;
}

error13_t d13_time13(time_t* t, char t13[MAXTIME]){
    int it13[D13_ITEMS];

    _d13_get_proper_sys_time(t, it13);

    d13_g2j(it13[0], it13[1], it13[2], it13);

    snprintf(	t13, MAXTIME, "%i-%i-%i-%i-%i-%i",
        it13[0], it13[1], it13[2], it13[3], it13[4], it13[5]
        );

    return E13_OK;
}

error13_t d13_time13g(time_t* t, char t13[MAXTIME]){
    int it13[D13_ITEMS];

    _d13_get_proper_sys_time(t, it13);

//    d13_g2j(it13[0], it13[1], it13[2], it13);

    snprintf(	t13, MAXTIME, "%i-%i-%i-%i-%i-%i",
        it13[0], it13[1], it13[2], it13[3], it13[4], it13[5]
        );

    return E13_OK;
}


//see comparision rules in header file.
int d13_cmp_time13(int t13_1[D13_ITEMS], int t13_2[D13_ITEMS]){

    register int i;

    for(i = 0; i < D13_ITEMS; i++){
        if(t13_1[i] && t13_2[i]){
            if(t13_1[i] > t13_2[i]){
                return 1;
            } else if(t13_1[i] < t13_2[i]){
                return -1;
            }
        }
    }

    return 0;

}

error13_t d13_clock(int* t13_time){

    time_t t;
    int it13[D13_ITEMS];

    if(time(&t) == -1){
        return e13_error(E13_SYSE);
    }

    _d13_get_proper_sys_time(&t, it13);

    *t13_time = it13[3]*10000+it13[4]*100+it13[5];

    return E13_OK;
}

error13_t d13_today(int it13[D13_ITEMS]){

    time_t t;

    if(time(&t) == -1){
        return e13_error(E13_SYSE);
    }

    _d13_get_proper_sys_time(&t, it13);

    d13_g2j(it13[0], it13[1], it13[2], it13);

    return E13_OK;
}

error13_t d13_todayg(int it13[D13_ITEMS]){

    time_t t;

    if(time(&t) == -1){
        return e13_error(E13_SYSE);
    }

    _d13_get_proper_sys_time(&t, it13);

//    d13_g2j(it13[0], it13[1], it13[2], it13);

    return E13_OK;
}

error13_t d13_now(char t13[MAXTIME]){
    time_t t;

    if(time(&t) == -1){
        return e13_error(E13_SYSE);
    }

    d13_time13(&t, t13);

    return E13_OK;
}

error13_t d13_nowg(char t13[MAXTIME]){
    time_t t;

    if(time(&t) == -1){
        return e13_error(E13_SYSE);
    }

    d13_time13g(&t, t13);

    return E13_OK;
}

error13_t d13_resolve_date(char* date, int d[D13_ITEMS]){
    char* start, *end;
    start = date;
    char datesep;
    int i;
    for(i = 0; i < D13_ITEMS; i++){

        end = strpbrk(start, d13_datesep);

        if(end) {
            datesep = *end;
            *end = 0;
        }

        d[i] = atoi(start);
        if(end) *end = datesep;
        if(d[i] == -1){
            return e13_error(E13_RANGE);
        }

        switch(i){
        case 0:
            break;
        case 1:
            if(d[1] > 12 || d[1] < 1) return e13_error(E13_RANGE);
            break;
        case 2:
            if(d[2] > 31 || d[2] < 1) return e13_error(E13_RANGE);
            break;
        case 3:
            if(d[3] > 24) return e13_error(E13_RANGE);
            break;
        case 4:
            if(d[4] > 60) return e13_error(E13_RANGE);
            break;
        case 5:
            if(d[5] > 60) return e13_error(E13_RANGE);
            break;
        }

        if(end) start = end + 1;
        else{
            if(i == 2) break;
            else return e13_error(E13_FORMAT);
        }
    }
    return E13_OK;
}

error13_t d13_fix_jdate(char *src, char date[MAXTIME]){

    int d[D13_ITEMS];
    error13_t e;
    char* prefix;

    if((e = d13_resolve_date(src, d)) != E13_OK){
        return e;
    }

    if(d[0] < 100){
        if(d[0] < 60) prefix = "14";//1400
        else prefix = "13";//1300's!
    } else prefix = NULL;

    snprintf(date, MAXTIME, "%s%i/%i/%i", prefix?prefix:"", d[0], d[1], d[2]);

    return E13_OK;
}

/*	--	serialize-able format functions	--	*/

error13_t d13s_clock(d13s_time_t* d13stime){

    time_t t;
    int it13[D13_ITEMS];
	char buf[MAXTIME];
	char* end;

    if(time(&t) == -1){
        return e13_error(E13_SYSE);
    }

    _d13_get_proper_sys_time(&t, it13);

	snprintf(buf, MAXTIME, "%0.4i%0.2i%0.2i%0.2i%0.2i%0.2i",
			it13[D13_ITEMS_YEAR],
			it13[D13_ITEMS_MON],
			it13[D13_ITEMS_DAY],
			it13[D13_ITEMS_HOUR],
			it13[D13_ITEMS_MIN],
			it13[D13_ITEMS_SEC]);

    //convert to the format

    *d13stime = strtoull(buf, &end, 10);

    _deb_serial("TIME: %s, CONVERTED: %llu", buf, *d13stime);

    return E13_OK;
}

error13_t _d13s_break(d13s_time_t stime, int d13time[D13_ITEMS]){
	int i;
	char buf[MAXTIME];
	char c;

    snprintf(buf, MAXTIME, "%llu", stime);

    d13time[D13_ITEMS_SEC] = atoi(buf+12);
    *(buf+12) = '\0';
	d13time[D13_ITEMS_MIN] = atoi(buf+10);
	*(buf+10) = '\0';
	d13time[D13_ITEMS_HOUR] = atoi(buf+8);
	*(buf+8) = '\0';
	d13time[D13_ITEMS_DAY] = atoi(buf+6);
	*(buf+6) = '\0';
	d13time[D13_ITEMS_MON] = atoi(buf+4);
	*(buf+4) = '\0';
	d13time[D13_ITEMS_YEAR] = atoi(buf);

	_deb_serial("%0.4i%0.2i%0.2i%0.2i%0.2i%0.2i",
				d13time[D13_ITEMS_YEAR],
				d13time[D13_ITEMS_MON],
				d13time[D13_ITEMS_DAY],
				d13time[D13_ITEMS_HOUR],
				d13time[D13_ITEMS_MIN],
				d13time[D13_ITEMS_SEC]);

    return E13_OK;
}

error13_t d13s_get_gtime(d13s_time_t stime, int gtime[D13_ITEMS]){

    return	_d13s_break(stime, gtime);

}

error13_t d13s_get_jtime(d13s_time_t t, int jtime[D13_ITEMS]){
    _d13s_break(t, jtime);
    if(!t) return E13_OK;
	return d13_g2j(	jtime[D13_ITEMS_YEAR],
					jtime[D13_ITEMS_MON],
					jtime[D13_ITEMS_DAY],
					jtime);
}

#ifdef TEST
int main(){

    time_t t;
    d13s_time_t stime;
    int gt[D13_ITEMS], jt[D13_ITEMS];

//    int conv = 0;//means g2j;
//    int in[3], out[3];
//    printf("enter conversion dir (0=g2j, 1=jtog): ");
//    scanf("%i", &conv);
//    printf("year: ");
//    scanf("%i", &in[0]);
//    printf("month: ");
//    scanf("%i", &in[1]);
//    printf("day: ");
//    scanf("%i", &in[2]);
//
//    if(!conv){
//        d13_g2j(in[0], in[1], in[2], out);
//    } else {
//        d13_j2g(in[0], in[1], in[2], out);
//    }
//
//    printf("out: %i / %i / %i\n", out[0], out[1], out[2]);

    d13s_clock(&stime);
	d13s_get_gtime(stime, gt);
	d13s_get_jtime(stime, jt);
    printf("\nstime: gtime, jtime: %llu: %i-%i-%i-%i-%i-%i, %i-%i-%i-%i-%i-%i\n",
			stime,
			gt[D13_ITEMS_YEAR],
			gt[D13_ITEMS_MON],
			gt[D13_ITEMS_DAY],
			gt[D13_ITEMS_HOUR],
			gt[D13_ITEMS_MIN],
			gt[D13_ITEMS_SEC],
			jt[D13_ITEMS_YEAR],
			jt[D13_ITEMS_MON],
			jt[D13_ITEMS_DAY],
			jt[D13_ITEMS_HOUR],
			jt[D13_ITEMS_MIN],
			jt[D13_ITEMS_SEC]);

	getchar(); getchar();
}
#endif

/*
 * StellaOS - rtc.c
 * Real-Time Clock (CMOS RTC) driver implementation
 */

#include "types.h"
#include "rtc.h"
#include "io.h"
#include "string.h"
#include "serial.h"

/* CMOS Register Addresses */
#define RTC_REG_SECONDS      0x00
#define RTC_REG_MINUTES      0x02
#define RTC_REG_HOURS        0x04
#define RTC_REG_WEEKDAY      0x06
#define RTC_REG_DAY          0x07
#define RTC_REG_MONTH        0x08
#define RTC_REG_YEAR         0x09
#define RTC_REG_STATUS_A     0x0A
#define RTC_REG_STATUS_B     0x0B
#define RTC_REG_CENTURY      0x32

static uint8_t rtc_read_register(uint8_t reg) {
    outb(RTC_PORT_INDEX, reg);
    return inb(RTC_PORT_DATA);
}

static int rtc_is_updating(void) {
    outb(RTC_PORT_INDEX, RTC_REG_STATUS_A);
    return (inb(RTC_PORT_DATA) & 0x80);
}

static uint8_t bcd_to_binary(uint8_t bcd) {
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

static void rtc_read_raw(uint8_t *sec, uint8_t *min, uint8_t *hour,
                         uint8_t *day, uint8_t *mon, uint8_t *yr,
                         uint8_t *wday, uint8_t *century) {
    /* Wait for Update In Progress (UIP) flag to clear */
    while (rtc_is_updating()) {
        io_wait();
    }

    *sec     = rtc_read_register(RTC_REG_SECONDS);
    *min     = rtc_read_register(RTC_REG_MINUTES);
    *hour    = rtc_read_register(RTC_REG_HOURS);
    *day     = rtc_read_register(RTC_REG_DAY);
    *mon     = rtc_read_register(RTC_REG_MONTH);
    *yr      = rtc_read_register(RTC_REG_YEAR);
    *wday    = rtc_read_register(RTC_REG_WEEKDAY);
    *century = rtc_read_register(RTC_REG_CENTURY);
}

static void rtc_read_stable(uint8_t *sec, uint8_t *min, uint8_t *hour,
                            uint8_t *day, uint8_t *mon, uint8_t *yr,
                            uint8_t *wday, uint8_t *century) {
    uint8_t last_sec, last_min, last_hour, last_day, last_mon, last_yr, last_wday, last_cent;

    /* Read once */
    rtc_read_raw(sec, min, hour, day, mon, yr, wday, century);

    /* Read repeatedly until two consecutive reads yield identical values */
    do {
        last_sec  = *sec;
        last_min  = *min;
        last_hour = *hour;
        last_day  = *day;
        last_mon  = *mon;
        last_yr   = *yr;
        last_wday = *wday;
        last_cent = *century;

        rtc_read_raw(sec, min, hour, day, mon, yr, wday, century);
    } while ((last_sec != *sec) || (last_min != *min) || (last_hour != *hour) ||
             (last_day != *day) || (last_mon != *mon) || (last_yr != *yr) ||
             (last_wday != *wday) || (last_cent != *century));

    uint8_t reg_b = rtc_read_register(RTC_REG_STATUS_B);

    /* Convert BCD to binary if required (bit 2 is 0 for BCD) */
    if (!(reg_b & 0x04)) {
        *sec     = bcd_to_binary(*sec);
        *min     = bcd_to_binary(*min);
        *hour    = (uint8_t)((( *hour & 0x7F ) > 0) ? bcd_to_binary(*hour & 0x7F) | (*hour & 0x80) : 0);
        *day     = bcd_to_binary(*day);
        *mon     = bcd_to_binary(*mon);
        *yr      = bcd_to_binary(*yr);
        *wday    = bcd_to_binary(*wday);
        if (*century != 0) {
            *century = bcd_to_binary(*century);
        }
    }

    /* Convert 12 hour clock to 24 hour clock if bit 1 of register B is 0 */
    if (!(reg_b & 0x02) && (*hour & 0x80)) {
        *hour = (uint8_t)(((*hour & 0x7F) + 12) % 24);
    }
}

void rtc_init(void) {
    char dt[32];
    rtc_get_datetime_string(dt, sizeof(dt));
    serial_printf("RTC: Initialized CMOS RTC. Current datetime: %s\n", dt);
}

void rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    uint8_t sec, min, hr, day, mon, yr, wday, cent;
    rtc_read_stable(&sec, &min, &hr, &day, &mon, &yr, &wday, &cent);

    if (hours)   *hours   = hr;
    if (minutes) *minutes = min;
    if (seconds) *seconds = sec;
}

void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *day) {
    uint8_t sec, min, hr, d, m, y, wday, cent;
    rtc_read_stable(&sec, &min, &hr, &d, &m, &y, &wday, &cent);

    if (year)  *year  = y;
    if (month) *month = m;
    if (day)   *day   = d;
}

uint16_t rtc_get_full_year(void) {
    uint8_t sec, min, hr, d, m, y, wday, cent;
    rtc_read_stable(&sec, &min, &hr, &d, &m, &y, &wday, &cent);

    if (cent != 0) {
        return (uint16_t)((cent * 100) + y);
    } else {
        /* Assume 2000s for year < 70, else 1900s */
        return (uint16_t)((y < 70 ? 2000 : 1900) + y);
    }
}

uint8_t rtc_get_day_of_week(void) {
    uint8_t sec, min, hr, d, m, y, wday, cent;
    rtc_read_stable(&sec, &min, &hr, &d, &m, &y, &wday, &cent);
    return wday;
}

void rtc_get_datetime_string(char *buf, size_t len) {
    if (!buf || len < 20) {
        if (buf && len > 0) buf[0] = '\0';
        return;
    }

    uint8_t sec, min, hr, day, mon, yr, wday, cent;
    rtc_read_stable(&sec, &min, &hr, &day, &mon, &yr, &wday, &cent);

    uint16_t full_year;
    if (cent != 0) {
        full_year = (uint16_t)((cent * 100) + yr);
    } else {
        full_year = (uint16_t)((yr < 70 ? 2000 : 1900) + yr);
    }

    /* Format: "YYYY-MM-DD HH:MM:SS" */
    buf[0]  = (char)('0' + ((full_year / 1000) % 10));
    buf[1]  = (char)('0' + ((full_year / 100) % 10));
    buf[2]  = (char)('0' + ((full_year / 10) % 10));
    buf[3]  = (char)('0' + (full_year % 10));
    buf[4]  = '-';
    buf[5]  = (char)('0' + ((mon / 10) % 10));
    buf[6]  = (char)('0' + (mon % 10));
    buf[7]  = '-';
    buf[8]  = (char)('0' + ((day / 10) % 10));
    buf[9]  = (char)('0' + (day % 10));
    buf[10] = ' ';
    buf[11] = (char)('0' + ((hr / 10) % 10));
    buf[12] = (char)('0' + (hr % 10));
    buf[13] = ':';
    buf[14] = (char)('0' + ((min / 10) % 10));
    buf[15] = (char)('0' + (min % 10));
    buf[16] = ':';
    buf[17] = (char)('0' + ((sec / 10) % 10));
    buf[18] = (char)('0' + (sec % 10));
    buf[19] = '\0';
}

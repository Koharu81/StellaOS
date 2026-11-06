/*
 * StellaOS - rtc.h
 * Real-Time Clock (CMOS RTC) driver interface
 */

#ifndef STELLA_RTC_H
#define STELLA_RTC_H

#include "types.h"

#define RTC_PORT_INDEX 0x70
#define RTC_PORT_DATA  0x71

/* RTC Public API */
void rtc_init(void);
void rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *day);
void rtc_get_datetime_string(char *buf, size_t len);
uint8_t rtc_get_day_of_week(void);
uint16_t rtc_get_full_year(void);

#endif /* STELLA_RTC_H */

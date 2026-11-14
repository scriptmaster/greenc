/*
 ** modified_time.c - 3 functions.
 ** calendar_day - Count Gregorian days without relying on the length of a local day.
 ** elapsed_period - Format a count in words.
 ** editor_modified_label - Select a relative label for a saved timestamp.
 ** See also: modified_time.h, editor_status.c.
 */
#include "modified_time.h"
#include "sys/builtin/types_intrinsic.h"
#include <stdio.h>

static i64 calendar_day(struct tm day) {
    i64 year = day.tm_year + 1900 - (day.tm_mon < 2);
    i32 month = (day.tm_mon + 10) % 12;

    return year * 365 + year / 4 - year / 100 + year / 400 + (153 * month + 2) / 5 + day.tm_mday;
}

static void elapsed_period(char label[64], i32 count, const char* unit) {
    static const char* const words[] = {"", "", "Two", "Three", "Four", "Five", "Six", "Seven",
                                        "Eight", "Nine", "Ten", "Eleven"};

    snprintf(label, 64, "%s %ss ago.", words[count], unit);
}

void editor_modified_label(char label[64], time_t modified, time_t now) {
    static const char* const weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                                           "Thursday", "Friday", "Saturday"};
    struct tm* local = localtime(&modified);
    if (!local) { snprintf(label, 64, "Unknown."); return; }
    struct tm past = *local;
    local = localtime(&now);
    if (!local) { snprintf(label, 64, "Unknown."); return; }

    i64 days = calendar_day(*local) - calendar_day(past);
    i32 months = (local->tm_year - past.tm_year) * 12 + local->tm_mon - past.tm_mon;
    if (local->tm_mday < past.tm_mday) { --months; }
    i32 years = months / 12;
    double seconds = difftime(now, modified);
    const char* phrase;
    if (seconds < 0) { phrase = "In the future."; }
    else if (days == 0) {
        phrase = seconds < 60 ? "A few seconds ago." : seconds < 3600 ? "A few minutes ago." : "A few hours ago.";
    } else if (days == 1) { phrase = "Yesterday."; }
    else if (days == 2) { phrase = "Day before yesterday."; }
    else if (days < 7) {
        snprintf(label, 64, "Last %s.", weekdays[past.tm_wday]);
        return;
    } else if (months <= 0 && days < 14) { phrase = "Last week."; }
    else if (months <= 0) { elapsed_period(label, (i32)(days / 7), "week"); return; }
    else if (months == 1) { phrase = "Last month."; }
    else if (months < 12) { elapsed_period(label, months, "month"); return; }
    else if (years == 1) { phrase = "Last year."; }
    else if (years < 10) { elapsed_period(label, years, "year"); return; }
    else if (years < 20) { phrase = "A decade ago."; }
    else if (years < 30) { phrase = "Two decades ago."; }
    else { phrase = "Many decades ago."; }

    snprintf(label, 64, "%s", phrase);
}

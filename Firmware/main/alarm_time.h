#ifndef ALARM_TIME
#define ALARM_TIME

// Set the alarm for all days
extern void set_alarms(int hour, int min);

// Set the alarm for a specific day
extern void set_alarm(int day_of_week, int hour, int min);

// Return the hour and min of the alarm for the given day of the week.
extern void get_alarm(int day_of_week, int *hour, int *min);

#endif //ALARM_TIME

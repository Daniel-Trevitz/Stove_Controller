#include "alarm_time.h"
#include "esp_log.h"
#include "nvs_flash.h"

struct Alarm
{
   int hour;
   int min;
};

// Set the alarm for all days
void set_alarms(int hour, int min)
{
    for(int i = 0; i < 7; i++)
    {
        set_alarm(i, hour, min);
    }
}

// Set the alarm for a specific day
void set_alarm(int day_of_week, int hour, int min)
{
    if(day_of_week < 0)
        return;

    if(day_of_week > 6)
        return;

    if((hour < 0) || (hour > 23))
        return;

    if((min < 0) || (min > 59))
        return;

    char dow[2];
    dow[0] = (char)day_of_week + (char)0x30;
    dow[1] = 0;

    printf("Setting day %s\n", dow);

    nvs_handle_t nvs_handle;
    int err = nvs_open("alarm", NVS_READONLY, &nvs_handle);
    if(err)
        return;

    struct Alarm alarm;
    alarm.hour = hour;
    alarm.min = min;

    err |= nvs_set_blob(nvs_handle, dow, &alarm, sizeof(struct Alarm));

    nvs_close(nvs_handle);

}

// Return the hour and min of the alarm for the given day of the week.
void get_alarm(int day_of_week, int *hour, int *min)
{
    if(day_of_week < 0)
        return;

    if(day_of_week > 6)
        return;

    if(!hour || !min)
        return;

    char dow[2];
    dow[0] = (char)day_of_week + (char)0x30;
    dow[1] = 0;

    printf("Getting day %s\n", dow);

    nvs_handle_t nvs_handle;
    int err = nvs_open("alarm", NVS_READONLY, &nvs_handle);
    if(err)
        return;

    struct Alarm alarm;
    size_t s = 0;

    err |= nvs_get_blob(nvs_handle, dow, &alarm, &s);

    nvs_close(nvs_handle);

    if(s == sizeof(struct Alarm))
    {
        *hour = alarm.hour;
        *min = alarm.min;
    }
    else
    {
        *hour = 0;
        *min = 0;
    }
}

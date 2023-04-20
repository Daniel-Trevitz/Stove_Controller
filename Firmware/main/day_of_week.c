int day_of_week()
{
    const int days_since_jan_1_leap_year[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
    const int days_since_jan_1_not_ly[]    = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    const int days_since_jan_1_2017_to_jan_of_this_year =  ((year - 2017)/4)*366 +  (year - 2017 - (year - 2017)/4)*365;

    const int months_in_days_this_year_to_this_month = year % 4 ? days_since_jan_1_not_ly[month-1] : days_since_jan_1_leap_year[month-1];

    const int days_since_jan_1_2017_to_today = day + days_since_jan_1_2017_to_jan_of_this_year + months_in_days_this_year_to_this_month - 1;

    return days_since_jan_1_2017_to_today % 7;
}

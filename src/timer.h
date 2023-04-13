#include <WiFiUdp.h>
#include <NTPClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void updateTime(){
    const int UTC_OFFSET_SUMMER = 7200;
    const int UTC_OFFSET_WINTER = 3600;

    timeClient.update();

    time_t rawtime = timeClient.getEpochTime();
    struct tm *ti = localtime(&rawtime);
  
    int month = ti->tm_mon + 1;
    int day = ti->tm_mday;
    int week_day = timeClient.getDay();

    // determine utc offset
    if(
      ((month == 3) && ((day-week_day) >= 25)) ||
      ((month > 3) && (month < 10)) ||
      ((month == 10) && ((day-week_day)) <= 24)
    ){
      timeClient.setTimeOffset(UTC_OFFSET_SUMMER);
    } else {
      timeClient.setTimeOffset(UTC_OFFSET_WINTER);
    }
}
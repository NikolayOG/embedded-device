#ifndef lastSunday_h
#define lastSunday_h

#include "Arduino.h"


class lastSunday
{
public:
	lastSunday();
	void findLastSunday(int y);
	void isleapyear();
	int getWeekDay( int m, int d );
	int getDay(int m);

private:
	int lastDay[12], year;
	boolean isleap;
};

#endif
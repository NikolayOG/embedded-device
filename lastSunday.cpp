#include "Arduino.h"
#include "lastSunday.h"

lastSunday::lastSunday()
{
}

void lastSunday::findLastSunday(int y)
{
  year = y;
  isleapyear();

  int days[] = { 31, isleap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
               d;
  for ( int i = 0; i < 12; i++ )
  {
    d = days[i];
    while ( true )
    {
      if ( !getWeekDay( i, d ) ) break;
      d--;
    }
    lastDay[i] = d;
  }

}

void lastSunday::isleapyear()
{
  isleap = false;
  if ( !( year % 4 ) )
  {
    if ( year % 100 ) isleap = true;
    else if ( !( year % 400 ) ) isleap = true;
  }
}

int lastSunday::getWeekDay( int m, int d )
{
  int y = year;

  int f = y + d + 3 * m - 1;
  m++;
  if ( m < 3 ) y--;
  else f -= int( .4 * m + 2.3 );

  f += int( y / 4 ) - int( ( y / 100 + 1 ) * 0.75 );
  f %= 7;

  return f;
}

int lastSunday::getDay(int m)
{
  return lastDay[m];
}

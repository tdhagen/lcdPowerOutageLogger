/*----------------------------------------------------------------------*
 * This is an implementation of the Sunrise/Sunset Algorithm found at   *
 * http://williams.best.vwh.net/sunrise_sunset_algorithm.htm            *
 * from the Almanac for Computers, 1990                                 *
 * Published by Nautical Almanac Office                                 *
 * Washington, DC 20392                                                 *
 * Implemented by Chris Snyder                                          *
 * Modified 09Dec2011 by Jack Christensen                               *
 *  - Improved rounding of the returned hour and minute values          *
 *    (e.g. would sometimes return 16h60m rather than 17h0m)            *
 *  - Replaced dayNumber() function with ordinalDate() and isLeap().    *
 *    (dayNumber returned zero as the first day for non-leap years.)    *
 *                                                                      *
 *    These changes resulted in better agreement with the US Naval      *
 *    Observatory calculations,                                         *
 *    http://aa.usno.navy.mil/data/docs/RS_OneYear.php.                 *
 *                                                                      *
 *    For 2011, for my locale, W083°37', N42°56', all sunrise and       *
 *    sunset times agreed within one minute. 12 sunrise times were one  *
 *    minute later than the USNO time, and 18 earlier. 19 sunset times  *
 *    were one minute later and 2 were earlier.                         *
 *----------------------------------------------------------------------*/

//#define OFFICIAL_ZENITH 90.83333  //  moved to _main
//#define CIVIL_ZENITH 96
//#define NAUTICAL_ZENITH 102
//#define ASTRONOMICAL_ZENITH 108

/*----------------------------------------------------------------------*
 * Function name:calcSunset
 * Params: $date(The day to calculate sunset or sunrise for)
 *	$lat(The latitude for the location to calculate sunrise or sunset for)
 *	$lon(The longitude for the location to calculate sunrise or sunset for west is negative)
 *	$sunset(if set to 1 calculates sunset if set to 0 sunrise)
 *	$GMToffset(difference in hours from GMT)
 *
 * 	$zenith: Sun's zenith for sunrise/sunset
 * 	  offical      = 90 degrees 50'  (90.8333)
 * 	  civil        = 96 degrees
 * 	  nautical     = 102 degrees
 * 	  astronomical = 108 degrees
 *
 * 	NOTE: longitude is positive for East and negative for West
 * 	      latitude is positive for North and negative for south
 *
 *----------------------------------------------------------------------*/

//#define pi 3.14159265358979323
#define pi 3.141593

float AdjustTo360 (float i)
{
    if (i > 360.0)
        i -= 360.0;
    else if (i < 0.0)
        i += 360.0;
    return i;
}

float AdjustTo24 (float i)
{
    if (i > 24.0)
        i -= 24.0;
    else if (i < 0.0)
        i += 24.0;
    return i;
}

float deg2rad (float degrees)
{
    return degrees * pi / 180;
}

float rad2deg (float radians)
{
    return radians / ( pi / 180 );
}

void calcSunset(int n, float lat, float lon, boolean sunset, float GMToffset, float zenith,
                byte &hour, byte &minutes)
{
  hour = minutes = 0;

    //Convert the longitude to hour value and calculate an approximate time.
    float lonhour = (lon/15);
    float t, m;

    if(sunset)
        t=n+((18 - lonhour)/24);
    else
        t=n+((6 - lonhour)/24);

    //Calculate the Sun's mean anomaly
    m = (0.9856*t) - 3.289;

    //Calculate the Sun's true longitude
    float sinm = sin(deg2rad(m));
    float sin2m = sin(2*deg2rad(m));
    float l= AdjustTo360 (m +(1.916 * sinm) + (0.02 * sin2m) + 282.634);

    //Calculate the Sun's right ascension(RA)
    float tanl = 0.91764 * tan(deg2rad(l));
    float ra = AdjustTo360 (rad2deg(atan(tanl)));

    //Putting the RA value into the same quadrant as L
    float lq = (floor(l/90)) * 90;
    float raq = (floor(ra/90)) * 90;
    ra = ra + (lq - raq);

    //Convert RA values to hours
    ra /= 15;

    //calculate the Sun's declination
    float sindec = 0.39782 * sin(deg2rad(l));
    float cosdec = cos(asin(sindec));

    //calculate the Sun's local hour angle
    float cosh = (cos(deg2rad(zenith)) - (sindec * sin(deg2rad(lat))))
        / (cosdec * cos(deg2rad(lat)));

    //if cosH > 1 the sun never rises on this date at this location
    //if cosH < -1 the sun never sets on this date at this location

    if (cosh >  1)
        return;
    else if (cosh < -1)
        return;

  float h;

    //finish calculating H and convert into hours
    if(sunset)
        h = rad2deg(acos(cosh));
    else
        h = 360 - rad2deg(acos(cosh));

    h /= 15;

    //Calculate local mean time of rising/setting
    t = h + ra - (0.06571 * t) - 6.622;
    
    //Adjust back to UTC
    float ut = AdjustTo24 (t - lonhour);
    //Adjust for current time zone
    ut= AdjustTo24 (ut+GMToffset) + 0.00833333;    //round up to the next minute by adding 30 seconds (0.00833333 hour) -- jc
    //was: ut= AdjustTo24 (ut+GMToffset);
    
    hour = floor(ut);
    minutes = 60.0 * (ut - hour);                  //rounded above, so letting the float-to-int assignment truncate is OK -- jc
    //was: minutes = round(60*(ut - hour));
}

//int days [] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
//
//int dayNumber (int year, int month, int day)
//{
//    int extra = 0;
//
//    if (floor (year % 4) == 0 && month > 2)
//        extra = 1;
//
//    return days [month - 1] + day + extra - 1;
//}

int ordinalDate(int y, int m, int d)
{
    if (m == 1)
        return d;
    else if (m == 2)
        return d + 31;
    else {
        int n = floor(30.6 * (m + 1)) + d - 122;    //see http://en.wikipedia.org/wiki/Ordinal_date#Calculation
        return n + (isLeap(y) ? 60 : 59);
    }
}

/*  redefinition - is in util tab
boolean isLeap(int y)
{
    //Leap years are those divisible by 4, but not those divisible by 100,
    //except that those divisble by 400 *are* leap years.
    //See Kernighan & Ritchie, 2nd edition, section 2.5.
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}
*/


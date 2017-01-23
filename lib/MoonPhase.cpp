#include <math.h>

#include "MoonPhase.h"

namespace MoonPhase {
/******************************************************************************/
// See: http://quasar.as.utexas.edu/BillInfo/JulianDatesG.html
/******************************************************************************/
double julian(int year, int month, double day) {
    if (month == 1 || month == 2) {
        year -= 1;
        month += 12;
    }

    double A = floor((double)year / 100.00);
    double B = floor(A / 4.0);
    double C = 2 - A + B;
    double E = floor(365.25 * (year + 4716));
    double F = floor(30.6001 * (month + 1));
    double JD = C + day + E + F - 1524.5;

    return (JD);
}
/******************************************************************************/
double sun_position(double j) {
    double n, x, e, l, dl, v;
    int i;

    n = 360 / 365.2422 * j;
    i = n / 360;
    n = n - i * 360.0;
    x = n - 3.762863;

    if (x < 0) {
        x += 360;
    };

    x *= RAD;
    e = x;

    do {
        dl = e - .016718 * sin(e) - x;
        e = e - dl / (1 - .016718 * cos(e));
    } while (fabs(dl) >= SMALL_FLOAT);

    v = 360 / M_PI * atan(1.01686011182 * tan(e / 2));
    l = v + 282.596403;
    i = l / 360;
    l = l - i * 360.0;

    return (l);
}
/******************************************************************************/
double moon_position(double j, double ls) {
    double ms, l, mm, n, ev, sms, ae, ec;
    int i;

    ms = 0.985647332099 * j - 3.762863;

    if (ms < 0)
        ms += 360.0;
    l = 13.176396 * j + 64.975464;
    i = l / 360;
    l = l - i * 360.0;
    if (l < 0)
        l += 360.0;
    mm = l - 0.1114041 * j - 349.383063;
    i = mm / 360;
    mm -= i * 360.0;
    n = 151.950429 - 0.0529539 * j;
    i = n / 360;
    n -= i * 360.0;
    ev = 1.2739 * sin((2 * (l - ls) - mm) * RAD);
    sms = sin(ms * RAD);
    ae = 0.1858 * sms;
    mm += ev - ae - 0.37 * sms;
    ec = 6.2886 * sin(mm * RAD);
    l += ev + ec - ae + 0.214 * sin(2 * mm * RAD);
    l = 0.6583 * sin(2 * (l - ls) * RAD) + l;

    return (l);
}
/******************************************************************************/
/*
      Calculates more accurately than Moon_phase , the phase of the moon at
      the given epoch.
      returns the moon phase as a real number (0-1)
*/
/******************************************************************************/
double moon_phase(int year, int month, int day, double hour) {
    double j = julian(year, month, (double)day + (hour / 24.0)) - 2444238.5;
    double ls = sun_position(j);
    double lm = moon_position(j, ls);
    double t = lm - ls;

    if (t < 0) {
        t += 360;
    }
    //    *ip = (int)((t + 22.5)/45) & 0x7;

    return (1.0 - cos((t)*RAD)) / 2;
}
};

#include "randlib.h"
#include <time.h>

bool shuffle(int start, int stop, int *data)
{
        if (start > stop) {
                return false;
        }

        int i, cnt, r;
        size_t n = stop - start;
        bool *idx = new bool[n+1];
        UniformRandom rnd(time(NULL));

        for (i=0; i<n+1; i++)
                idx[i] = false;

        cnt = 0;
        while (cnt <= n) {
                r = round(start + n*rnd.doub());
                if (!idx[r-start]) {
                        idx[r-start] = true;
                        data[cnt] = r;
                        cnt++;
                }
        }

        delete idx;
        return true;
}

double gammln(double x) {
	int j;
	double xx, tmp, y, ser;
	static const double cof[14] = {
		57.1562356658629235     , -59.5979603554754912     ,
		14.1360979747417471     ,  -0.491913816097620199   ,
		 0.339946499848118887e-4,   0.465236289270485756e-4,
		-0.983744753048795646e-4,   0.158088703224912494e-3,
		-0.210264441724104883e-3,   0.217439618115212643e-3,
		-0.164318106536763890e-3,   0.844182239838527433e-4,
		-0.261908384015814087e-4,   0.368991826595316234e-5
	};
	
	if (x <= 0) 
		throw("bad arg in gammln");
	
	y = xx = x;
	tmp = xx+5.24218750000000000; // Rational 671/128.
	tmp = (xx+0.5) * log(tmp) - tmp;
	ser = 0.999999999999997092;
	for (j=0; j<14; j++)
		ser += cof[j] / ++y;
	return tmp + log(2.5066282746310005*ser/xx);
}


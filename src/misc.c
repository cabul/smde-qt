#include "simulator.h"

#include <float.h>
#include <stdlib.h>

#include "utils.h"

void draw_dist(FILE *f, struct dist *dist) {
	const int nrolls = 10000;
	const int nstars = 100;
	const int nintervals = 10;

	int p[nintervals];
	for (int i=0; i<nintervals; ++i) p[i] = 0;

	double *r = malloc(sizeof(double)*nrolls);
	memcheck(r);

	double rmax = DBL_MIN;
	double rmin = DBL_MAX;

	for (int i=0; i<nrolls; ++i) {
		r[i] = rand_dist(dist);
		rmax = max(rmax, r[i]);
		rmin = min(rmin, r[i]);
	}

	for (int i=0; i<nrolls; ++i) {
		double rnorm = (r[i]-rmin)/(rmax-rmin);
		if (rnorm < 1.0) ++p[(int)(rnorm * (double)nintervals)];
		else ++p[nintervals-1];
	}

	print_dist(f, dist);

	for (int i=0; i<nintervals; ++i) {
		fprintf(f, "%d: ", i);
		for (int j=0; j<p[i]*nstars/nrolls; ++j) fprintf(f, "*");
		fprintf(f, "\n");
	}
}

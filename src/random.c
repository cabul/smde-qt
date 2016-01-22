#include "simulator.h"
#include "utils.h"

#include <stdlib.h>
#include <math.h>

#define NLAYERS 256

double rand_unif(double a, double b) {
	double r = (double)rand()/(double)RAND_MAX;
	return a + r * (b-a);
}

double rand_norm(double mean, double dev) {
	double rsum = 0;
	for (int i=0; i<NLAYERS; ++i) {
		rsum += rand_unif(0,1);
	}
	rsum /= NLAYERS;
	rsum -= 0.5;
	rsum *= sqrt(12*NLAYERS);
	return mean + dev * rsum;
}

double rand_erlang(int shape, double rate) {
	double r = 1;
	for (int i=0; i<shape; ++i) r *= rand_unif(0,1);
	return log(r)/-rate;
}

double rand_exp(double rate) {
	double u = rand_unif(0,1);
	return log(1-u)/-rate;
}

double rand_hypo(double a, double b) {
	die("Not supported");
	return 0;
}

double rand_dist(struct dist *dist) {
	switch(dist->type) {
		case CONS: return dist->data.cons.value;
		case UNIF: return rand_unif(dist->data.unif.a, dist->data.unif.b);
		case NORM: return rand_norm(dist->data.norm.mean, dist->data.norm.dev);
		case ERLANG: return rand_erlang(dist->data.erlang.shape, dist->data.erlang.rate);
		case HYPO: return rand_hypo(dist->data.hypo.a, dist->data.hypo.b);
		default: return 0;
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"
#include "simulator.h"

int main (int argc, char *argv[]) {
	check(argc >= 2, "usage: %s <datafile> [<niter>] [<rho>]", argv[0]);

	FILE *f = fopen(argv[1], "r");
	check(f, "Unable to open %s for reading", argv[1]);

	struct params params;
	parse_params(f, &params);

	fclose(f);

	int niter = (argc>2) ? atoi(argv[2]) : 1000;
	check(niter>0, "Invalid value for niter %s", argv[2]);

	double rho = (argc>3) ? atof(argv[3]) : 0.2;

	//srand(time(NULL));
	srand(1993);

	print_params(stderr, &params);
	fprintf(stderr, "tau distribution:\n");
	draw_dist(stderr, &params.dist_tau);
	fprintf(stderr, "x distribution:\n");
	draw_dist(stderr, &params.dist_x);
	fprintf(stderr, "c distribution:\n");
	draw_dist(stderr, &params.dist_c);

	struct dist dist_U = {
		.type = ERLANG,
		.data.erlang = {
			.shape = 1,
			.rate = 1 / params.mu;
		}
	};

	struct cycle *cycles = malloc(sizeof(struct cycle) * niter);
	memcheck(cycles);
	
	double X_mean  = 0;
	double Y_mean  = 0;
	double dev_tau = 0;
	double dev_c   = 0;
	double dev_z   = 0;
	cycles[0].x    = 0;
	cycles[0].X    = 0;
	cycles[0].T    = 0;

	double sim_time = 0.0;
	double time_limit = 0.0;

	for (int i=0; i<niter-1; ++i) {
		cycles[i].tau = rand_dist(&params.dist_tau);
		cycles[i].tau = min(cycles[i].x, cycles[i].tau);
		cycles[i+1].x = rand_dist(&params.dist_x);
		cycles[i+1].c = rand_dist(&params.dist_c);
		int A_1 = (sim_time > time_limit) ? 1 : 0;
		time_limit += cycles[i].tau - cycles[i].x;
		while (sim_time <= time_limit) {
			double arrival_time = rand_dist(&dist_U);
			sim_time += arrival_time;
			++A_1;
		}
		if (sim_time > time_limit) --A_1;
		int A_2 = (sim_time > time_limit) ? 1 : 0;
		time_limit += cycles[i+1].x;
		while (sim_time <= time_limit) {
			double arrival_time = rand_dist(&dist_U);
			sim_time += arrival_time;
			++A_2;
		}
		if (sim_time > time_limit) --A_2;
		cycles[i].A = A_1 + A_2;
		cycles[i+1].T = cycles[i].T + cycles[i].tau;
		cycles[i].Y = A_1;
		cycles[i+1].X = cycles[i].Y
	}

	free(cycles);
	return 0;
}

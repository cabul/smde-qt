#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

#include "utils.h"
#include "simulator.h"

int main (int argc, char *argv[]) {
	int opt, opt_index;
	FILE *f = NULL;
	
	struct params params;
	params.niter = 100;
	params.rho   = 0.5;
	params.seed  = time(NULL);
	
	static struct option options[] =  {
		{"help",   no_argument,       0, 'h'},
		{"file",   required_argument, 0, 'f'},
		{"rho",    required_argument, 0, 'r'},
		{"cycles", required_argument, 0, 'c'},
		{"seed",   required_argument, 0, 's'},
		{0, 0, 0, 0}
	};
	
	while ((opt = getopt_long(argc, argv, "hf:r:c:s:", options, &opt_index)) != -1) {
		switch (opt) {
			case 'h':
				printf("Usage: %s --file f [--rho r] [--cycles c] [--seed s]\n", argv[0]);
				return 0;
			case 'f':
				f = fopen(optarg, "r");
				check(f, "Unable to open %s for reading", optarg);
				parse_params(f, &params);
				fclose(f);
				break;
			case 'r':
				params.rho = atoi(optarg);
				check(params.rho > 0, "Rho have to be a positive real value");
				break;
			case 'c':
				params.niter = atoi(optarg);
				check(params.niter > 0, "Cycles have to be a positive integer number");
				break;
			case 's':
				params.seed = atof(optarg);
				break;
		}
	}
	
	check(f, "An input file describing the bulk system is needed");
	
	srand(params.seed);

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
			.rate = 1 / params.mu
		}
	};

	struct cycle *cycles = malloc(sizeof(struct cycle) * params.niter);
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

	for (int i=0; i<params.niter-1; ++i) {
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
		cycles[i+1].X = cycles[i].Y;
	}

	free(cycles);
	return 0;
}

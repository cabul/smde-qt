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

	struct dist dist_U = {  // 1-erlang, exp
		.type = ERLANG,
		.data.erlang = {
			.shape = 1,
			.rate = 1 / params.mu
		}
	};

	struct cycle *cycles = malloc(sizeof(struct cycle) * params.niter);
	memcheck(cycles);

	// Statistics

	double dev_tau  = 0.0;
	double dev_c    = 0.0;
	double dev_z    = 0.0;
	double total_A  = 0.0;
	double total_W  = 0.0;
	double total_Wp = 0.0;
	double total_Z  = 0.0;
	double total_S  = 0.0;
	double X_max    = 0.0; // X tilde (not mean)
	double X_min    = 0.0;
	double sum_X    = 0.0;
	double dev_X    = 0.0;
	double Y_max    = 0.0;
	double Y_min    = 0.0;
	double sum_Y    = 0.0;
	double dev_Y    = 0.0;
	double gamma    = 0.0; // The served clients
	double gamma_c  = 0.0;
	double dev_g    = 0.0;

	// Setup queue simulator

	cycles[0].x    = 0.0;
	cycles[0].X    = 0.0;
	cycles[0].T    = 0.0;

	double sim_time = 0.0;
	double time_limit = 0.0;

	// TODO: to draw the evolution of the queue we can print 
	// sim_time,queuesize each Ti,Ti+xi
	for (int i=0; i<params.niter-1; ++i) {
		cycles[i].tau = rand_dist(&params.dist_tau);
		cycles[i].tau = max(cycles[i].x, cycles[i].tau);   // time between servers arrive
		cycles[i+1].T = cycles[i].T + cycles[i].tau;       // server arrives
		cycles[i+1].x = rand_dist(&params.dist_x);         // service time of server
		cycles[i+1].c = rand_dist(&params.dist_c);         // service's capacity

		int A_1 = (sim_time > time_limit) ? 1 : 0;         // extra arrival added to A1
		time_limit += cycles[i].tau - cycles[i].x;         // limit time for A1
		while (sim_time <= time_limit) {
			double arrival_time = rand_dist(&dist_U);
			sim_time += arrival_time;
			++A_1;
		}
		if (sim_time > time_limit) --A_1;                  // extra arrival for A1

		int A_2 = (sim_time > time_limit) ? 1 : 0;         // extra arrival added to A2
		time_limit += cycles[i+1].x;                       // timi limit for A2
		while (sim_time <= time_limit) {
			// I think we mixed the client arrival distribution
			// with the client serving distribution (?)
			// Last paragraph page 1
			double arrival_time = rand_dist(&dist_U);
			sim_time += arrival_time;
			++A_2;
		}
		if (sim_time > time_limit) --A_2;                  // extra arrival for A2

		cycles[i].A = A_1 + A_2;                           // total client arrivals of cycle

		// TODO: missing vars Wqi, Di, Wi, Wpi

		cycles[i+1].S = 0;                                 // 0 served clients at the beginning
		time_limit = cycles[i+1].x;
		while (sim_time <= time_limit) {                   // serve clients
			sim_time += rand_dist(&dist_U);                // TODO: checkthisout.png
			++cycles[i+1].S;
		}
		if (sim_time > time_limit) --cycles[i+1].S;        // whoopsie

		cycles[i+1].X = max(
			cycles[i].X + cycles[i].A - min(
				cycles[i+1].S, cycles[i+1].c), 
			0);                                            // non-served clients
		cycles[i+1].z = min(
			min(cycles[i].X + cycles[i].A, cycles[i+1].c), 
			cycles[i+1].S);                                // served clients
		cycles[i+1].T = cycles[i].T + cycles[i].tau;       // time of server arrival
		cycles[i].Y   = cycles[i].X + A_1;                 // clients before server arrival

		// Evaluate statistics

		dev_tau  += cycles[i].tau * cycles[i].tau;
		dev_c    += cycles[i+1].c * cycles[i+1].c;
		dev_z    += cycles[i+1].z * cycles[i+1].z;
		total_A  += cycles[i].A;
		total_W  += cycles[i].W;
		total_Wp += cycles[i].Wp;
		total_Z  += cycles[i+1].z;
		total_S  += cycles[i+1].S;
		X_max     = max(X_max, cycles[i+1].X);
		X_min     = min(X_min, cycles[i+1].X);
		sum_X    += cycles[i+1].X;
		dev_X    += cycles[i+1].X * cycles[i+1].X;
		gamma    += min(cycles[i].S, cycles[i+1].c);
		gamma_c  += cycles[i+1].c;
		dev_g    += min(cycles[i+1].S * cycles[i+1].S, cycles[i+1].c * cycles[i+1].c);
		Y_max     = max(Y_max, cycles[i].Y);
		Y_min     = min(Y_min, cycles[i].Y);
		sum_Y    += cycles[i].Y;
		dev_Y    += cycles[i].Y * cycles[i].Y;

		// Update

		cycles[i].X = cycles[i+1].X;
		cycles[i].x = cycles[i+1].x;
		cycles[i].T = cycles[i+1].T;
		cycles[i].tau = cycles[i+1].tau;
	}

	free(cycles);
	return 0;
}

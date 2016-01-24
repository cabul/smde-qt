#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <math.h>

#include "utils.h"
#include "simulator.h"

void generate(double t, struct dist *dist, double *n, double *W) {
	*n = 0;
	*W = 0.0;
	double theta = 0.0;

	while (theta < t) {
		double dtheta = rand_dist(dist);
		*W += *n * (min(t, theta+dtheta) - theta);
		theta += dtheta;
		if (theta < t) *n += 1;
	}
}

int main (int argc, char *argv[]) {
	int opt, opt_index;
	FILE *infile = NULL;
	FILE *outfile = stdout;

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
		{"output",   required_argument, 0, 'o'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "hf:r:c:s:o:", options, &opt_index)) != -1) {
		switch (opt) {
			case 'h':
				printf("Usage: %s --file f [--rho r] [--cycles c] [--seed s]\n", argv[0]);
				return 0;
			case 'f':
				infile = fopen(optarg, "r");
				check(infile, "Unable to open %s for reading", optarg);
				parse_params(infile, &params);
				fclose(infile);
				break;
			case 'r':
				params.rho = atof(optarg);
				check(params.rho > 0, "Rho have to be a positive real value");
				break;
			case 'c':
				params.niter = atoi(optarg);
				check(params.niter > 0, "Cycles have to be a positive integer number");
				break;
			case 's':
				params.seed = atof(optarg);
				break;
			case 'o':
				outfile = fopen(optarg, "w");
				check(outfile, "Unable to open %s for writing", optarg);
		}
	}

	check(infile, "An input file describing the bulk system is needed");

	srand(params.seed);

	print_params(stderr, &params);
	fprintf(stderr, "tau distribution:\n");
	draw_dist(stderr, &params.dist_tau);
	fprintf(stderr, "x distribution:\n");
	draw_dist(stderr, &params.dist_x);
	fprintf(stderr, "c distribution:\n");
	draw_dist(stderr, &params.dist_c);

	// prettyfy this
	struct dist dist_U = {
		.type = ERLANG,
		.data.erlang = {
			.shape = 1,
			.rate = 1 / (params.rho * params.mu) 
		}
	};
	fprintf(stderr, "U distribution:\n");
	draw_dist(stderr, &dist_U);

	struct dist dist_eps = {  // 1-erlang, exp
		.type = ERLANG,
		.data.erlang = {
			.shape = 1,
			.rate = params.mu
		}
	};
	
	fprintf(stderr, "eps distribution:\n");
	draw_dist(stderr, &dist_eps);

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

	// TODO: to draw the evolution of the queue we can print 
	// sim_time,queuesize each Ti,Ti+xi
	for (int i=0; i<params.niter-1; ++i) {
		cycles[i].tau = rand_dist(&params.dist_tau);
		cycles[i].tau = max(cycles[i].x, cycles[i].tau);   // time between servers arrive
		cycles[i+1].x = rand_dist(&params.dist_x);         // service time of server
		cycles[i+1].c = round(rand_dist(&params.dist_c));         // service's capacity

		double A_1;
		generate(cycles[i].tau - cycles[i].x, &dist_U, &A_1, &cycles[i].Wq);
		debug("A_1 %g", A_1);

		double A_2;
		generate(cycles[i+1].x, &dist_U, &A_2, &cycles[i].D);
		debug("A_2 %g", A_2);

		cycles[i].A = min(A_1 + A_2, params.Amax);         // total client arrivals of cycle
		cycles[i].W = cycles[i].Wq + cycles[i].X * cycles[i].tau + A_1 * cycles[i+1].x + cycles[i].D;
		cycles[i].Wp = cycles[i].Wq + cycles[i].X * cycles[i].tau + cycles[i].D;
		
		double dummy;
		generate(cycles[i+1].x, &dist_eps, &cycles[i+1].S, &dummy);
		debug("Si %g", cycles[i+1].S);
		
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
	
	fprintf(outfile, "time,value\n");
	for (int i = 0; i < params.niter-1; ++i) {
		fprintf(outfile, "%g,%g\n", cycles[i].T + cycles[i].x, cycles[i].X);
		fprintf(outfile, "%g,%g\n", cycles[i+1].T, cycles[i].Y);
	}

	if (outfile != stdout) fclose(outfile);
	free(cycles);
	return 0;
}

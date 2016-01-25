#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>

#include "utils.h"
#include "simulator.h"

// Generate a number n of occurences of dist over time t
// The accumulated time is saved in W
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
	FILE *outfile = NULL;

	struct params params;
	params.niter = 100;
	params.rho   = 0.5;
	params.seed  = time(NULL);

	bool verbose = false;

	static struct option options[] =  {
		{"help",    no_argument,       0, 'h'},
		{"file",    required_argument, 0, 'f'},
		{"rho",     required_argument, 0, 'r'},
		{"cycles",  required_argument, 0, 'c'},
		{"seed",    required_argument, 0, 's'},
		{"output",  required_argument, 0, 'o'},
		{"verbose", no_argument,       0, 'v'},
		{0, 0, 0, 0}
	};

	// Parse parameters
	while ((opt = getopt_long(argc, argv, "hf:r:c:s:o:v", options, &opt_index)) != -1) {
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
				break;
			case 'v':
				verbose = true;
				break;
		}
	}

	check(infile, "An input file describing the bulk system is needed");

	srand(params.seed);

	// We can calculate lambda based on the given parameters
	//
	//             E[A]            where:    E[A] = lambda x E[tau]
	// rho = ----------------                E[s] = mu x E[x]
	//        min{E[c],E[S]}
	//
	double lambda = params.rho * min(params.Ec, params.mu * params.Ex) / params.Etau;

	// Distribution for arrival times of clients
	struct dist dist_U = {
		.type = EXP,
		.data.exp = {
			.rate = lambda
		}
	};
	
	if (verbose) {
	}

	// Distribution for the service times (normal vs. exponential)
#ifdef USE_NORM
	struct dist dist_eps = {
		.type = NORM,
		.data.norm = {
			.mean = 1   / params.mu,
			.dev  = 0.1 / params.mu
		}
	};
#else
	struct dist dist_eps = {
		.type = EXP,
		.data.exp = {
			.rate  = params.mu
		}
	};
#endif
	
	if (verbose) {
		printf("Lambda: %g\n", lambda);
		printf("Mu: %g\n", params.mu);
		print_params(stderr, &params);
		fprintf(stderr, "tau distribution:\n");
		draw_dist(stderr, &params.dist_tau);
		fprintf(stderr, "x distribution:\n");
		draw_dist(stderr, &params.dist_x);
		fprintf(stderr, "c distribution:\n");
		draw_dist(stderr, &params.dist_c);
		fprintf(stderr, "U distribution:\n");
		draw_dist(stderr, &dist_U);
		fprintf(stderr, "eps distribution:\n");
		draw_dist(stderr, &dist_eps);
	}

	struct cycle *cycles = malloc(sizeof(struct cycle) * params.niter);
	memcheck(cycles);

	// Statistics

	double sigma_tau = 0.0;
	double sigma_c   = 0.0;
	double sigma_z   = 0.0;
	double sigma_x   = 0.0;
	double sigma_Y   = 0.0;
	double sigma_g   = 0.0;
	double sigma_X   = 0.0;
	double sigma_S   = 0.0;
	double total_A   = 0.0;
	double total_W   = 0.0;
	double total_Wp  = 0.0;
	double total_z   = 0.0;
	double total_S   = 0.0;
	double total_x   = 0.0;
	double total_tau = 0.0;
	double total_Y   = 0.0;
	double total_X   = 0.0;
	double total_g   = 0.0; // The served clients
	double total_c   = 0.0;
	double X_max     = 0.0; // X tilde (not mean)
	double X_min     = DBL_MAX;
	double Y_max     = 0.0;
	double Y_min     = DBL_MAX;

	// Setup queue simulator

	cycles[0].x    = 0.0;
	cycles[0].X    = 0.0;
	cycles[0].T    = 0.0;

	for (int i=0; i<params.niter-1; ++i) {
		cycles[i].tau = rand_dist(&params.dist_tau);
		cycles[i].tau = max(cycles[i].x, cycles[i].tau);   // time between servers arrive
		cycles[i+1].x = rand_dist(&params.dist_x);         // service time of server
		cycles[i+1].c = round(rand_dist(&params.dist_c));         // service's capacity

		double A_1;
		generate(cycles[i].tau - cycles[i].x, &dist_U, &A_1, &cycles[i].Wq);

		double A_2;
		generate(cycles[i+1].x, &dist_U, &A_2, &cycles[i].D);

		cycles[i].A = min(A_1 + A_2, params.Amax);         // total client arrivals of cycle
		cycles[i].W = cycles[i].Wq + cycles[i].X * cycles[i].tau + A_1 * cycles[i+1].x + cycles[i].D;
		cycles[i].Wp = cycles[i].Wq + cycles[i].X * cycles[i].tau + cycles[i].D;
		
		double dummy;
		generate(cycles[i+1].x, &dist_eps, &cycles[i+1].S, &dummy);
		
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

		sigma_tau += cycles[i].tau * cycles[i].tau;
		sigma_S   += cycles[i+1].S * cycles[i+1].S;
		sigma_c   += cycles[i+1].c * cycles[i+1].c;
		sigma_z   += cycles[i+1].z * cycles[i+1].z;
		sigma_X   += cycles[i+1].X * cycles[i+1].X;
		sigma_x   += cycles[i+1].x * cycles[i+1].x;
		sigma_g   += min(cycles[i+1].S * cycles[i+1].S, cycles[i+1].c * cycles[i+1].c);
		sigma_Y   += cycles[i].Y * cycles[i].Y;

		total_A   += cycles[i].A;
		total_W   += cycles[i].W;
		total_Wp  += cycles[i].Wp;
		total_z   += cycles[i+1].z;
		total_S   += cycles[i+1].S;
		total_x   += cycles[i+1].x;
		total_tau += cycles[i].tau;
		total_X   += cycles[i+1].X;
		total_Y   += cycles[i].Y;
		total_g   += min(cycles[i].S, cycles[i+1].c);
		total_c   += cycles[i+1].c;

		X_max      = max(X_max, cycles[i+1].X);
		X_min      = min(X_min, cycles[i+1].X);
		Y_max      = max(Y_max, cycles[i].Y);
		Y_min      = min(Y_min, cycles[i].Y);

		// Update

		cycles[i].X = cycles[i+1].X;
		cycles[i].x = cycles[i+1].x;
		cycles[i].T = cycles[i+1].T;
		cycles[i].tau = cycles[i+1].tau;
	}

	double N = params.niter+0.0;
	double TN = cycles[params.niter-1].T;
	double xN = cycles[params.niter-1].x;

	double sd_tau = sqrt( (N/(N-1.0)) * (sigma_tau/N - (TN*TN)/(N*N)));
	double W0 = TN/(2*N) * (1.0 + (N*N*sd_tau*sd_tau)/(TN*TN));

	printf("rho:      %.2f\n", total_A/min(total_g, total_S));
	printf("mean tau: %.2f\n", total_tau/params.niter);
	printf("sd   tau: %.2f\n", sd_tau);
	printf("W0:       %.2f\n", W0);
	printf("mean x:   %.2f\n", total_x/params.niter);
	printf("sd   x:   %.2f\n", sqrt( (N/(N-1.0)) * (sigma_x/N - (total_x*total_x)/(N*N))));
	printf("mean c:   %.2f\n", total_c/params.niter);
	printf("sd   c:   %.2f\n", sqrt( (N/(N-1.0)) * (sigma_c/N - (total_c*total_c)/(N*N))));
	printf("mean S:   %.2f\n", total_S/params.niter);
	printf("sd   S:   %.2f\n", sqrt( (N/(N-1.0)) * (sigma_S/N - (total_S*total_S)/(N*N))));
	printf("mu:       %.2f\n", total_S/total_x);
	printf("mean A:   %.2f\n", total_A/params.niter);
	printf("L:        %.2f\n", total_W / (TN+xN));
	printf("L':       %.2f\n", total_Wp / (TN+xN));
	printf("mean X:   %.2f\n", total_X/params.niter);
	printf("max  X:   %.2f\n", X_max);
	printf("min  X:   %.2f\n", X_min);
	printf("sd   X:   %.2f\n", sqrt( (N/(N-1.0)) * (sigma_X/N - (total_X*total_X)/(N*N))));
	printf("mean Y:   %.2f\n", total_Y/params.niter);
	printf("max  Y:   %.2f\n", Y_max);
	printf("min  Y:   %.2f\n", Y_min);
	printf("sd   Y:   %.2f\n", sqrt( (N/(N-1.0)) * (sigma_Y/N - (total_Y*total_Y)/(N*N))));
	printf("mean z:   %.2f\n", total_z/params.niter);
	printf("sd   z:   %.2f\n", sqrt( (N/(N-1.0)) * (sigma_z/N - (total_z*total_z)/(N*N))));
	total_W /= (N*(total_A/N));
	total_Wp /= (N*(total_A/N));
	printf("W/W0:     %.2f\n", total_W/W0);
	printf("W'/W0:    %.2f\n", total_Wp/W0);
	
	// Print trace of execution
	if (outfile) {
		fprintf(outfile, "time,queue\n");
		fprintf(outfile, "0,0\n");
		fprintf(outfile, "%g,%g\n", cycles[0].T + cycles[0].x, cycles[0].X);
		for (int i = 1; i < params.niter-1; ++i) {
			fprintf(outfile, "%g,%g\n", cycles[i].T, cycles[i-1].Y);
			fprintf(outfile, "%g,%g\n", cycles[i].T + cycles[i].x, cycles[i].X);
		}
		fclose(outfile);
	}

	free(cycles);
	return 0;
}

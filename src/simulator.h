#pragma once

#include "stdio.h"

struct cycle {
	double tau;
	double T;
	double x;
	double A;
	double W;
	double Wq;
	double D;
	double Wp;
	double X;
	double Y;
	double c;
	double S;
	double z;
};

enum dist_type {
	CONS, UNIF, ERLANG, NORM, HYPO
};

union dist_data {
	struct {
		double value;
	} cons;
	struct {
		double a;
		double b;
	} unif;
	struct {
		int shape;
		double rate;
	} erlang;
	struct {
		double mean;
		double dev;
	} norm;
	struct {
		double a;
		double b;
	} hypo;
};

struct dist {
	enum dist_type type;
	union dist_data data;
};

struct params {
	double Etau;
	struct dist dist_tau;
	double Ex;
	struct dist dist_x;
	double Ec;
	struct dist dist_c;
	double C;
	double mu;
	double Amax;
	double rho;
	int niter;
	int seed;
};

void parse_dist(FILE *f, struct dist *dist);
void print_dist(FILE *f, struct dist *dist);
void parse_params(FILE *f, struct params *params);
void print_params(FILE *f, struct params *params);

double rand_unif(double a, double b);
double rand_norm(double mean, double dev);
double rand_erlang(int shape, double rate);
double rand_exp(double rate);
double rand_hypo(double a, double b);

double rand_dist(struct dist *dist);

void draw_dist(FILE *f, struct dist *dist);

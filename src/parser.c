#include "simulator.h"
#include "utils.h"

void parse_dist(FILE *f, struct dist *dist) {
	char id[10];
	int n;
	n = fscanf(f, "%s", id);
	check(n, "Unable to parse distribution");

	if (streq(id, "CONS")) {
		dist->type = CONS;
		n = fscanf(f, "%lf", &dist->data.cons.value);
		check(n, "Unable to parse value for CONS");
	} else if (streq(id, "UNIF")) {
		dist->type = UNIF;
		n = fscanf(f, "%lf", &dist->data.unif.a);
		check(n, "Unable to parse a for UNIF");
		n = fscanf(f, "%lf", &dist->data.unif.b);
		check(n, "Unable to parse b for UNIF");
	} else if (streq(id, "ERLANG")) {
		dist->type = ERLANG;
		n = fscanf(f, "%d", &dist->data.erlang.shape);
		check(n, "Unable to parse shape for ERLANG");
		n = fscanf(f, "%lf", &dist->data.erlang.rate);
		check(n, "Unable to parse rate for ERLANG");
	} else if (streq(id, "NORM")) {
		dist->type = NORM;
		n = fscanf(f, "%lf", &dist->data.norm.mean);
		check(n, "Unable to parse mean for NORM");
		n = fscanf(f, "%lf", &dist->data.norm.dev);
		check(n, "Unable to parse dev for NORM");
	} else if (streq(id, "HYPO")) {
		n = fscanf(f, "%lf", &dist->data.hypo.a);
		check(n, "Unable to parse a for HYPO");
		n = fscanf(f, "%lf", &dist->data.hypo.b);
		check(n, "Unable to parse b for HYPO");
		dist->type = HYPO;
	} else die("Unkown distribution %s", id);
}

void print_dist(FILE *f, struct dist *dist) {
	switch(dist->type) {
		case CONS:
			fprintf(f, "CONS %lf\n", dist->data.cons.value);
			break;
		case UNIF:
			fprintf(f, "UNIF %lf %lf\n", dist->data.unif.a, dist->data.unif.b);
			break;
		case ERLANG:
			fprintf(f, "ERLANG %d %lf\n", dist->data.erlang.shape, dist->data.erlang.rate);
			break;
		case NORM:
			fprintf(f, "NORM %lf %lf\n", dist->data.norm.mean, dist->data.norm.dev);
			break;
		case HYPO:
			fprintf(f, "HYPO %lf %lf\n", dist->data.hypo.a, dist->data.hypo.b);
			break;
	}
}

void parse_params(FILE *f, struct params *params) {
	int n;
	n = fscanf(f, "%lf", &params->Etau);
	check(n, "Unable to parse Etau");
	parse_dist(f, &params->dist_tau);
	n = fscanf(f, "%lf", &params->Ex);
	check(n, "Unable to parse Ex");
	parse_dist(f, &params->dist_x);
	n = fscanf(f, "%lf", &params->Ec);
	check(n, "Unable to parse Ec");
	parse_dist(f, &params->dist_c);
	n = fscanf(f, "%lf", &params->C);
	check(n, "Unable to parse C");
	n = fscanf(f, "%lf", &params->mu);
	check(n, "Unable to parse mu");
	n = fscanf(f, "%lf", &params->Amax);
	check(n, "Unable to parse Amax");
}

void print_params(FILE *f, struct params *params) {
	fprintf(f, "%lf\n", params->Etau);
	print_dist(f, &params->dist_tau);
	fprintf(f, "%lf\n", params->Ex);
	print_dist(f, &params->dist_x);
	fprintf(f, "%lf\n", params->Ec);
	print_dist(f, &params->dist_c);
	fprintf(f, "%lf\n", params->C);
	fprintf(f, "%lf\n", params->mu);
	fprintf(f, "%lf\n", params->Amax);
}

# A realistic bulk service system

Implementation of a bulk system simulator for the course Statistical modelling and design of experiments of 2015/16.

## Instructions

To build:

	$ make             # Uses the exponential distribution for service times
	$ make -dUSE_NORM  # Uses the normal distribution for service times

To run:

	$ bin/simulator -f <input file>
	               [-s <seed>] [-o <output>]
	               [-c <cycles>] [-r <rho>]
	               [-v] [-h]

The input file has to be in the following format:

	E[tau]
	PD(tau)
	E[x]
	PD(x)
	E[c]
	PD(c)
	C
	mu
	Amax

Where _PD(x)_ is the probability distributions used for variable _x_. The supported distributions are the following (values in <> are numbers):

	CONS <num>            # Constant value
	UNIF <a> <b>          # Uniform distribution
	NORM <mean> <sd>      # Normal distribution
	EXP <rate>            # Exponential distribution
	ERLANG <shape> <rate> # k-Erlang distribution

To get a representation based on the trace file run the following command (Note: R must be installed):

	$ make images/sth.png    # Assumes traces/sth.csv exists

You can also run these scripts directly:

	$ utils/script.R in.csv out.png    # Used for the command above 
	$ SEEDS=`seq 1 10` CYCLES=100 utils/table.sh  # Generates a table with values for different seeds

The structure of the project is the following:

	src/       Source files
	build/     Build files  
	bin/       Location of executable
	utils/     Utility scripts
	data/      Sample input data
	traces/    Location of execution traces
	images/    Visual representation of traces

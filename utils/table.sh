#!/bin/bash

[[ -z $SEEDS ]] && SEEDS=(`seq 1 10`)
[[ -z $CYCLES ]] && CYCLES=100
RHO=0.98

N=${#SEEDS[@]}
echo $N

printf "value"
for SEED in ${SEEDS[@]}
do
	printf ",$SEED"
done
printf ",mean,sd\n"

getkey() {
	KEY=$1
	printf $KEY
	VALS=()
	ACC=0
	for SEED in ${SEEDS[@]}
	do
		VAL=$(bin/simulator -f data/calvin.dat -r $RHO -s $SEED -c $CYCLES | grep "$KEY:" | sed 's/[^:]*:\s*//')
		ACC=$(echo "$ACC + $VAL" | bc -l)
		VALS=("${VALS[@]}" "$VAL")
		printf ",$VAL"
	done
	MEAN=$(echo "$ACC / $N" | bc -l)
	SD=0
	for VAL in ${VALS[@]}
	do
		SD=$(echo "$SD + ($VAL-$MEAN)*($VAL-$MEAN)" | bc -l)
	done
	SD=$(echo "$SD / $N" | bc -l)
	printf ",$MEAN,$SD\n"
}

getkey "rho"
getkey "L"
getkey "mean Y"
getkey "mean A"
getkey "W/W0"


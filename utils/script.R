#!/usr/bin/env Rscript
args = commandArgs(trailingOnly = TRUE);
table = read.csv(file = args[1])
png(args[2], width=800, heigh=600, units="px")
plot(table$time, table$queue, cex=0.1, xlab="Time", ylab="Queue", type='l');

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define die(M, ...) \
	do { fprintf(stderr, "[Error] " M , ##__VA_ARGS__); fprintf(stderr, " (%s:%d)\n", __FILE__, __LINE__); exit(EXIT_FAILURE); } while(0)

#define check(A, M, ...) \
	do { if (!(A)) { fprintf(stderr, "[Error] " M , ##__VA_ARGS__); fprintf(stderr, " (%s:%d)\n", __FILE__, __LINE__); exit(EXIT_FAILURE); } } while(0)

#define memcheck(M) check(M, "Unable to allocate memory")

#ifndef NDEBUG
#define debug(M, ...) \
	do { fprintf(stderr, "[DEBUG] " M , ##__VA_ARGS__); fprintf(stderr, " (%s:%d)\n", __FILE__, __LINE__); } while(0)
#else
#define debug(M, ...)
#endif

#define warn(M, ...) \
	do { fprintf(stderr, "[WARN] " M , ##__VA_ARGS__); fprintf(stderr, " (%s:%d)\n", __FILE__, __LINE__); } while(0)

#define cdiv(A,B) ((A)/(B) + (((A)%(B))!=0))
#define min(A,B) ((A)<(B) ? (A) : (B))
#define max(A,B) ((A)>(B) ? (A) : (B))
#define streq(A,B) (strcmp((A),(B)) == 0)

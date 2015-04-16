/*
	How to use getopt()

	The idea is to replace DASM's command line arguments
	parsing with something standardized. This might cost
	us some Windows support I guess. Oh well... :-)
*/

#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <getopt.h>

struct configuration {
	char *listing_file;
	char *object_file;
	int object_format;
	int verbosity;
	bool debug;
	char **input_files;
	int num_input_files;
} config;

static const char *short_options = "l:o:f:vdh?";

static const struct option long_options[] = {
	{"listing-file", required_argument, NULL, 'l'},
	{"object-file", required_argument, NULL, 'o'},
	{"format", required_argument, NULL, 'f'},
	{"verbose", no_argument, NULL, 'v'},
	{"debug", no_argument, NULL, 'd'},
	{"help", no_argument, NULL, 'h'},
	{NULL, no_argument, NULL, 0}
};

int main(int argc, char **argv)
{
	config.listing_file = NULL;
	config.object_file = "./a.out";
	config.object_format = 1;
	config.verbosity = 0;
	config.debug = false;
	config.num_input_files = 0;
	config.input_files = NULL;

	int option = getopt_long(argc, argv, short_options, long_options, NULL);
	while (option != -1) {
		switch (option) {
		case 'l':
			config.listing_file = strdup(optarg);
			break;
		case 'o':
			config.object_file = strdup(optarg);
			break;
		case 'f':
			config.object_format = atoi(optarg);
			break;
		case 'v':
			config.verbosity += 1;
			break;
		case 'd':
			config.debug = true;
			break;
		case 'h':
		case '?':
			printf("help / usage messages here\n");
			return EXIT_FAILURE;
			break;
		default:
			assert(false);
			break;
		}
		option = getopt_long(argc, argv, short_options, long_options, NULL);
	}

	config.input_files = argv + optind;
	config.num_input_files = argc - optind;

	printf("listing_file = %s\n", config.listing_file);
	printf("object_file = %s\n", config.object_file);
	printf("object_format = %d\n", config.object_format);
	printf("verbosity = %d\n", config.verbosity);
	printf("debug = %d\n", config.debug);
	printf("num_input_files = %d\n", config.num_input_files);
	for (int i = 0; i < config.num_input_files; i++) {
		printf("input_file[%d] = %s\n", i, config.input_files[i]);
	}

	return EXIT_SUCCESS;
}

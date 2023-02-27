#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bigmath.h"

#define LINE_LEN (256 * sizeof(char))

void fileopen(FILE** file, char const* name, char const* mode, char** line)
{
	errno_t err;
	if ((err = fopen_s(file, name, mode)) != 0)
	{
		strerror_s(*line, LINE_LEN, err);
		fprintf(stderr, "Cannot open file %s: %s\n", name, *line);
		exit(0);
	}
}

void basechange(FILE* inptr, FILE* outptr, char* line, size_t* line_number, unsigned short from_base, unsigned short work_base, BigInt* a)
{
	// Base validation
	if (from_base < 2 || from_base > 16)
	{
		fprintf(stderr, "[%zu] Invalid from_base: %hu. Base must be in range [2, 16]\n", *line_number, from_base);
		longjmp(exception, 1);
	}

	if (work_base < 2 || work_base > 16)
	{
		fprintf(stderr, "[%zu] Invalid to_base: %hu. Base must be in range [2, 16]\n", *line_number, work_base);
		longjmp(exception, 1);
	}

	// Structure validation: one empty line, number, two empty lines
	for (size_t i = 0; i < 4; ++i)
	{
		++*line_number;
		// Get line
		if (fgets(line, 256, inptr) == NULL)
		{
			fprintf(stderr, "Unexpected EOF\n");
			exit(0);
		}

		if(i == 1) // Number line - strip '\n' and convert to number
		{
			strip(line);
			stobig(line, from_base, a);
		}
		else // Empty line expected
		{
			if (line[0] != '\n')
			{
				fprintf(stderr, "[%zu] Missing empty line!\n", *line_number);
				fprintf(stderr, "Correct format: <from_base> <to_base>\n\n<number>\n\n\n");
				longjmp(exception, 1);
			}
		}
	}

	// Verbose info to console
	printf("%hu -> %hu\n", from_base, work_base);
	bigprint(a, from_base, stdout);
	putc('\n', stdout);

	// Print result to file 
	fprintf(outptr, "%hu %hu\n\n", from_base, work_base);
	bigprint(a, from_base, outptr);
	fputc('\n', outptr);
	bigprint(a, work_base, outptr);
	fputc('\n', outptr);
}

void evaluateop(char operation, BigInt* a, BigInt* b, BigInt* res)
{
	switch (operation)
	{
	case '+': // a + b
		bigadd(a, b, res);
		break;
	case '*': // a * b
		bigmul(a, b, res);
		break;
	case '/': // a / b
		bigquo(a, b, res);
		break;
	case '%': // a % b
		bigmod(a, b, res);
		break;
	case '^': // a ^ b
		bigpow(a, b, res);
		break;
	default: // Invalid operation
		fprintf(stderr, "Uknown operation: %c\n", operation);
		longjmp(exception, 1);
	}
}

void calculation(FILE* inptr, FILE* outptr, char* line, size_t* line_number, char operation, unsigned short work_base, BigInt* a, BigInt* b, BigInt* res)
{
	// Structure validation: one empty line, number A, one empty line, number B, two empty lines
	for (size_t i = 0; i < 6; ++i) {
		++*line_number;
		// Get line
		if (fgets(line, 256, inptr) == NULL)
		{
			fprintf(stderr, "Unexpected EOF\n");
			exit(0);
		}

		switch (i)
		{
		case 1: // Number A line - strip '\n' and convert to number
			strip(line);
			stobig(line, work_base, a);
			break;
		case 3: // Number B line - strip '\n' and convert to number
			strip(line);
			stobig(line, work_base, b);
			break;
		default: // Empty line expected
			if (line[0] != '\n')
			{
				fprintf(stderr, "[%zu] Missing empty line!\n", *line_number);
				fprintf(stderr, "Correct format: <operation> <base>\n\n<number>\n\n<number>\n\n\n");
				longjmp(exception, 1);
			}
		}
	}

	// Verbose info to console
	printf("[%hu]\n", work_base);
	bigprint(a, work_base, stdout);
	printf("%c\n", operation);
	bigprint(b, work_base, stdout);
	putc('\n', stdout);

	// Print beggining of result to file 
	fprintf(outptr, "%c %hu\n\n", operation, work_base);
	bigprint(a, work_base, outptr);
	putc('\n', outptr);
	bigprint(b, work_base, outptr);
	putc('\n', outptr);

	// Compute result
	evaluateop(operation, a, b, res);

	// Print actual result to file
	bigprint(res, work_base, outptr);
	putc('\n', outptr);
}

int main(int argc, char** argv)
{
	FILE* inptr, * outptr;
	errno_t err;

	char* outname = "result.txt";
	char* line = (char*)malloc(LINE_LEN);
	size_t len = 0;
	size_t line_number = 0;

	unsigned short from_base;
	char operation;
	unsigned short work_base;
	BigInt a = _zero;
	BigInt b = _zero;
	BigInt res = _zero;

	// Command line validation
	if (argc < 2)
	{
		fprintf(stderr, "Filename required! Usage: calculate <input> [output=result.txt]\n");
		return 0;
	}

	// Optional output file argument
	if (argc == 3)
		outname = argv[2];

	// Input file initialization
	fileopen(&inptr, argv[1], "r", &line);

	// Output file initialization
	fileopen(&outptr, outname, "w", &line);

	// Main program loop
	while (fgets(line, 256, inptr) != NULL)
	{
		line_number++;

		// Exception handling using <setjmp.h>
		if (setjmp(exception) == 0)
		{
			// Case 1: base conversion <from> <to>
			if (sscanf_s(line, "%hu %hu", &from_base, &work_base) == 2)
			{
				basechange(inptr, outptr, line, &line_number, from_base, work_base, &a);
			}
			else if (sscanf_s(line, "%c %hu", &operation, 1, &work_base) == 2) // Case 2: operation <op_symbol> <base>
			{
				// Base validation
				if (work_base < 2 || work_base > 16)
				{
					fprintf(stderr, "[%zu] Invalid base: %hu. Base must be in range [2, 16]\n", line_number, work_base);
					continue;
				}

				calculation(inptr, outptr, line, &line_number, operation, work_base, &a, &b, &res);
			}
			else // Unknown line format
			{
				strip(line);
				fprintf(stderr, "[%zu] Cannot understand line: '%s'\n", line_number, line);
			}
		}
		else // Exception detected either in structure or during computation
		{
			fprintf(stderr, "[%zu] An error occured during calculation!\n", line_number);
			fprintf(outptr,"An error occured during calculation!\n");
		}

		// Free values
		freeval(a);
		freeval(b);
		freeval(res);
	}

	// Cleanup - close files and free memory
	fclose(inptr);
	fclose(outptr);
	cleanup();
	free(line);

	return 0;
}

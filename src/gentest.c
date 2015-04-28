#include "asm.h"
#include "errors.h"

/**
 * @file
 * Generate test cases for mnemonics/addressing modes.
 *
 * Allows us to generate simple test case files directly from DASM's internal
 * tables. The files can be used (with minor edits) to cross-validate what we
 * accept against other assemblers. See src/main.c for how it's activated.
 */

/**
 */
static void generate_addressing_mode(FILE *tst, int a)
{
	switch (a) {
	case AM_IMP:
		fprintf(tst, "\t");
		fprintf(tst, "\t\t; implied [%d]\n", a);
		break;
	case AM_IMM8:
		fprintf(tst, "\t#$0c");
		fprintf(tst, "\t\t; immediate8 [%d]\n", a);
		break;
	case AM_IMM16:
		fprintf(tst, "\t#$c011");
		fprintf(tst, "\t\t; immediate16 [%d]\n", a);
		break;
	case AM_BYTEADR:
		fprintf(tst, "\t$a0");
		fprintf(tst, "\t\t; address8 [%d]\n", a);
		break;
	case AM_BYTEADRX:
		fprintf(tst, "\t$a1,x");
		fprintf(tst, "\t\t; address8,x [%d]\n", a);
		break;
	case AM_BYTEADRY:
		fprintf(tst, "\t$a2,y");
		fprintf(tst, "\t\t; address8,y [%d]\n", a);
		break;
	case AM_WORDADR:
		fprintf(tst, "\t$afa3");
		fprintf(tst, "\t\t; address16 [%d]\n", a);
		break;
	case AM_WORDADRX:
		fprintf(tst, "\t$afa4,x");
		fprintf(tst, "\t\t; address16,x [%d]\n", a);
		break;
	case AM_WORDADRY:
		fprintf(tst, "\t$afa5,y");
		fprintf(tst, "\t\t; address16,y [%d]\n", a);
		break;
	case AM_REL:
		fprintf(tst, "\t$40");
		fprintf(tst, "\t\t; relative8 [%d]\n", a);
		break;
	case AM_INDBYTEX:
		fprintf(tst, "\t($70,x)");
		fprintf(tst, "\t\t; (address8,x) [%d]\n", a);
		break;
	case AM_INDBYTEY:
		fprintf(tst, "\t($71),y");
		fprintf(tst, "\t\t; (address8),y [%d]\n", a);
		break;
	case AM_INDWORD:
		fprintf(tst, "\t($d00d)");
		fprintf(tst, "\t\t; (address16) [%d]\n", a);
		break;
	case AM_0X:
		fprintf(tst, "\t???");
		fprintf(tst, "\t\t; ???zero,x??? [%d]\n", a);
		break;
	case AM_0Y:
		fprintf(tst, "\t???");
		fprintf(tst, "\t\t; ???zero,y??? [%d]\n", a);
		break;
	case AM_BITMOD:
		fprintf(tst, "\t???");
		fprintf(tst, "\t\t; ???bitmod??? [%d]\n", a);
		break;
	case AM_BITBRAMOD:
		fprintf(tst, "\t???");
		fprintf(tst, "\t\t; ???bitbramod??? [%d]\n", a);
		break;
	default:
		fprintf(tst, "\t???");
		fprintf(tst, "\t\t; OUT OF RANGE? [%d]\n", a);
		break;
	}
}

/**
 */
static void generate_instructions(FILE *tst, MNEMONIC *table, bool valid)
{
	for (MNEMONIC *i = table; i->name != NULL; i++) {
		for (int a = AM_IMP; a < NUMOC; a++) {
			bool ok = (i->okmask & (1L << a)) != 0;

			if (!valid) {
				ok = !ok;
			}

			if (ok) {
				fprintf(tst, "\t%s", i->name);
				generate_addressing_mode(tst, a);
			}
		}
	}
}

/**
 * Generate a test file with the given name for the forced processor.
 *
 * If valid is true we'll generate the valid addressing modes only;
 * if it's false we'll generate the invalid addressing modes instead.
 */
void generate_test_file(const char* filename, bool valid)
{
	if (!processor_forced) {
		panic_fmt("Force processor with -m to get a test file.");
	}
	if (selected_processor == NULL) {
		panic_fmt("Cannot create test file for unknown processor.");
	}

	FILE *tst = fopen(filename, "w");
	if (tst == NULL) {
		panic_fmt("Failed to create file '%s' for test cases.",
				filename);
	}

	fprintf(tst, "\t.processor %s\n", selected_processor->name);
	fprintf(tst, "\t.org\t$0\n");

        for (MNEMONIC **m = selected_processor->mnemonic_tables;
			*m != NULL; m++) {
		generate_instructions(tst, *m, valid);
        }

	fprintf(tst, "\t.end\n");

	fclose(tst);
}

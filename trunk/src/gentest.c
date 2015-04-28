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
 * Write a fake for the actual addressing mode in a form that DASM will
 * accept again.
 */
static void generate_addressing_mode(FILE *tst, int a)
{
	/* TODO: the huge switch/print orgy is ugly, I hope to
	 * come up with a better way of doing this [phf] */
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
		/* [phf] copied Matt's trick of using . for branches */
		fprintf(tst, "\t.");
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
	/*
	 * [phf] The following two addressing modes seem inspired by the
	 * 68705; only the ,x mode is available on that machine BTW,
	 * I believe the ,y mode was invented by Matthew Dillon just
	 * for symmetry reasons. If you can find it anywhere "for real"
	 * please let me know. Maybe we can support yet another CPU
	 * cheaply that way. :-) The syntax is horrid, just ",x" with
	 * nothing else. But hey, that's what we're stuck with. I wonder
	 * what the actual Motorola syntax for this was.
	 */
	case AM_0X:
		fprintf(tst, "\t,x");
		fprintf(tst, "\t\t; implied,x (aka address0 aka zero-offset) [%d]\n", a);
		break;
	case AM_0Y:
		fprintf(tst, "\t,y");
		fprintf(tst, "\t\t; implied,y (aka address0 aka zero-offset) [%d]\n", a);
		break;
	/*
	 * These two addressing modes *also* seem to come from the 68705
	 * and they are fascinating because they have *three* arguments.
	 * That's something I thought impossible before when I discussed
	 * the broken HD6303 opcodes which would also need three. I need
	 * to investigate these *very* closely.
	 *
	 * Alright, these are really weird. So there's a bit number that
	 * we test for which is encoded into the actual opcode. (I don't
	 * even know if DASM does that!) Then there's a "direct" address
	 * which is the 68705's way of saying "zero page" I guess. That's
	 * all for bclr and bset. For brclr and brset there's a *third*
	 * byte that specifies the branch offset. So as per usual there's
	 * a difference between the signed branch offset and the actual
	 * branch range; the offset is -128 to 127, the branch offset is
	 * -125 to 130; because the instruction is *three* bytes long as
	 * opposed to a 6502 branch that's only two bytes long. Again I
	 * wonder if DASM actually deals with this. Let's look.
	 *
	 * Amazing! There are special cases for these in ops.c/v_mnemonic()
	 * but there are also new MF_ flags to mark the instructions as
	 * special. Looking into this showed that mne6811.c *also* have
	 * these kinds of instructions but with different MF_ flags. It's
	 * fascinating! I might have to add more cases below to properly
	 * generate the fake addressing mode string. [phf]
	 */
	case AM_BITMOD:
		fprintf(tst, "\t1,23");
		fprintf(tst, "\t\t; ???bitmod??? [%d]\n", a);
		break;
	case AM_BITBRAMOD:
		/* [phf] copied Matt's trick of using . for branches */
		fprintf(tst, "\t1,23,.");
		fprintf(tst, "\t\t; ???bitbramod??? [%d]\n", a);
		break;
	default:
		fprintf(tst, "\t???");
		fprintf(tst, "\t\t; ???invalid addressing mode??? [%d]\n", a);
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

; $Id: jentzsch2.asm 299 2008-11-10 02:17:18Z phf $
;
; Thomas Jentzsch <tjentzsch@yahoo.de> test case for bug
; expression evaluation.

	processor 6502
	ORG $100

	echo (.)d, "works"
	echo (*)d, "works"
	echo (1 - .)d, "works"
	echo (1 - *)d, "works"
	echo (1 - . - 1)d, "works"
	echo (1 - * - 1)d, "works"
	echo (1 - 1 - .)d, "works"
	echo (1 - 1 - *)d, "broken"

	echo (1 + *)d, "works"
	echo (1 + * + 1)d, "works"
	echo (1 + 1 + *)d, "broken"

	END

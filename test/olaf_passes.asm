; A test requiring multiple passes to resolve.
;
; I found a version of this in dasm.txt when I needed something
; to verify DASM's behavior when lots of passes are needed. You
; will understand this better if you remember this:
;
;   "a ? b : c" in C is "[a ? b - c] + c" in DASM
;
; Peter H. Froehlich
; phf at acm dot org

	.processor	6502
	.org		1

	.repeat		[[x < 11] ? [x-11]] + 11
	dc.b		x
	.repend
x:
	.end

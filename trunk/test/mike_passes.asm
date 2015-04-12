; A branch requiring multiple passes to resolve.
;
; This is from Mike Saarna illustrating the need for his new
; REASON code for another pass. Strangely enough it works for
; me without his fix. :-/
;
; Peter H. Froehlich
; phf at acm dot org

	.processor 6502
test = temp
temp = $90
	.org $f000
start:
	.repeat $39
	sta test
	.repend
check:
	bne start
branch:
	bne branch
end:
	.end

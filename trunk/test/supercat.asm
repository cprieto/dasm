; $Id$
;
; Pretty horrible bug... :-/
;
; From "supercat" on the atariage.com forums, forwarded by
; Andrew Davie 2008/04/12.
;
; Peter H. Froehlich
; phf at acm dot org

	.processor 6502

	org	$7FF8
	byte	1,2,3,4,5

; original line by "supercat", disabled to avoid producing
; huge files by accident...
;	ds	$7FFC-*

; Peter's line making the problem tractable: a negative number
; is taken as a huge positive one during expression evaluation,
; so "ds" above tries to produce a huge object file... :-/
	long	$7FFC-*

	byte	1,2,3,4

	.end

; The kind of "force" directive we want to fail. Only
; the first dc.w should be alright, all others should
; error out nicely. *Maybe* the second one can be let
; go since .wx is valid for *some* instructions. Meh!
; Whoever wrote the code for .force... :-/

	.processor 6502
	.org $f666
start:
	dc.w	64
	dc.wx	64
	dc.ww	64
	dc.www	64
	dc.www.dasmsucks.org	64
ohboy:
	.end

; Brian Watson reported that this triggers an assert() in
; DASM's exit handler. I need to check my understanding of
; the rmnode() function.

	processor 6502
	org $0
	if 1<2
		err
	endif

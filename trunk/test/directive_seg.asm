; Tests to see how weird segment names work out. Turns out
; they do work out alright. I don't like it, but hey, it's
; alright. :-) You need -f2 to assemble this properly.

	.processor 6502

	; put some code into the default segment
	.org $0
	sta $cccc
	rts

	; create a named segment
	.seg bla
	.org $100
	sta $dddd
	rts

	; create another named segment, with a space in the name
	.seg another segment
	.org $200
	sta $eeee
	rts

	; create an anonymous segment
	.seg
	.org $300
	sta $ffff
	rts

	; get back to the named segment
	.seg bla
	lda #$42

	; get back to the other segment
	.seg another segment
	lda #$52

	; get back to the anonymous segment
	.seg
	lda #$62

	; get back to the initial segment
	.seg INITIAL CODE SEGMENT
	lda #$72

	end

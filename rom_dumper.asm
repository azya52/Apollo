	.byte $00

	* = $0036

	ldx	#$00
	lda #$00
bankLoop:
	sta $D7
byteLoop:
	lda (romPtr, X)
	rol
	ldx	#$08
bitLoop:
	and #$FE
	sta $C3
	ora #$01
	sta $C3
	ror
	dex
	bne bitLoop

	inc romPtrL
	bne byteLoop
	inc romPtrH
	lda romPtrH
	cmp #$20
	bne byteLoop
	
	lda #$10
	sta romPtrH

	inc bank
	lda bank
	cmp #$04
	bne bankLoop

	brk
	
romPtr:
romPtrL:
	.byte $00
romPtrH:
	.byte $00
bank:
	.byte $00
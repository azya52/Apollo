	.byte $00

    * = $36

	ldx #$36
	txs
	jsr clearLCD

	lda #$01
    sta $D4
main:
	ldx #00		       
mainLoop:
	lda #$02
	sta $C3
	lda #$00
	sta $C3
	lda $C1
	sta $D5

	ror
	lda segStates             
  	rol
	bcc nextByte
	sta $00, X
  	inx

	lda #$02
  	sta $C3
	lda #$01
	sta $C3
	sta segStates
	lda $C1
	sta $D5

	ror
	rol segStates

	cpx #64
	beq main
	jmp mainLoop
nextByte:
	sta segStates
	jmp mainLoop

segStates:
	.byte 01

	* = $055B
clearLCD:
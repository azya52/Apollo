    dataSize = 74

    .byte 00

    * = $0E
    ldx #dataSize-1
mainLoop:
	inc loBitPtr+1
	clc
loop8:
loBitPtr:
	ror loBit-1
	beq mainLoop
	lda data, X
	rol
	pha
	dex
	bmi start
	jmp loop8
loBit:
	;lower bits of each data byte, 7 bits per byte
	* = 120-dataSize
data:
	;right shifted data
	* = 128-dataSize
start:


test: berzerk.bin
	-diff berzerk.bin berzerk.bin.ref
	-diff berzerk.asm berzerk.bin.ref
	-diff berzerk.bin berzerk.bin.ref

berzerk.bin: berzerk.asm
	../../bin/dasm berzerk.asm -f3 -I../../machines/atari2600/ -oberzerk.bin

clean:
	rm -rf berzerk.bin

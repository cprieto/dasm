test: berzerk.bin boing26.bin
	-diff berzerk.bin berzerk.bin.ref
	-diff boing26.bin boing26.bin.ref

berzerk.bin: berzerk.asm
	../../bin/dasm berzerk.asm -f3 -I../../machines/atari2600/ -oberzerk.bin
boing26.bin: boing26.asm
	../../bin/dasm boing26.asm -f3 -I../../machines/atari2600/ -oboing26.bin

clean:
	rm -rf berzerk.bin boing26.bin

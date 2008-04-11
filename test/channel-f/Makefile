test: lights.bin
	-diff lights.bin lights.bin.ref

lights.bin: lights.asm
	../../bin/dasm lights.asm -f3 -I../../machines/channel-f/ -olights.bin

clean:
	rm -rf lights.bin

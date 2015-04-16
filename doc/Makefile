# Gentoo installs "rst2man.py" and friends but Ubuntu installs
# "rst2man" and so on. Better look for the commands ourselves.
# A bit naively maybe?
RST2MAN := $(shell /usr/bin/find /usr/bin -name "rst2man*")
RST2HTML := $(shell /usr/bin/find /usr/bin -name "rst2html*")
RST2LATEX := $(shell /usr/bin/find /usr/bin -name "rst2latex*")

all: ensuretools
	@echo "Nothing to do yet."

viewman:
	$(RST2MAN) dasm.rst | nroff -man | less
	$(RST2MAN) ftohex.rst | nroff -man | less

.PHONY: ensuretools
ensuretools:
ifndef RST2MAN
	$(error rst2man is not installed)
endif
ifndef RST2HTML
	$(error rst2html is not installed)
endif
ifndef RST2LATEX
	$(error rst2latex is not installed)
endif

.PHONY: clean
clean:
	@echo "Eventually this will clean house."

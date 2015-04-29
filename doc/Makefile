# Gentoo installs "rst2man.py" and friends but Ubuntu installs
# "rst2man" and so on. Better look for the commands ourselves.
# 'which' will search the user's $PATH.
RST2MAN   := $(shell which rst2man   2>/dev/null || which rst2man.py   2>/dev/null )
RST2HTML  := $(shell which rst2html  2>/dev/null || which rst2html.py  2>/dev/null )
RST2LATEX := $(shell which rst2latex 2>/dev/null || which rst2latex.py 2>/dev/null )

all: ensuretools
	@echo "Nothing to do yet."

viewman:
	$(RST2MAN) dasm.rst | nroff -man | less
	$(RST2MAN) ftohex.rst | nroff -man | less

viewpdf:
	$(RST2LATEX) dasm.rst > dasm.tex
	pdflatex dasm.tex
	pdflatex dasm.tex
	mupdf dasm.pdf
	$(RST2LATEX) ftohex.rst > ftohex.tex
	pdflatex ftohex.tex
	pdflatex ftohex.tex
	mupdf ftohex.pdf

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
	rm -rf *.aux *.log *.pdf *.tex *.out

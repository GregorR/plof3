PDFLATEX=pdflatex

all: letter.pdf

world: letter.pdf letterbook.pdf a4.pdf a4book.pdf

.SUFFIXES: .tex .pdf

.tex.pdf:
	pdflatex $< && pdflatex $<

g: letter.pdf
	evince letter.pdf

k: letter.pdf
	okular letter.pdf

clean:
	rm -f *.aux *.log *.pdf *.toc

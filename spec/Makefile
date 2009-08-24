PDFLATEX=pdflatex

all: letter.pdf

world: letter.pdf letterbook.pdf a4.pdf a4book.pdf

letter.pdf: hgversion
letterbook.pdf: hgversion
a4.pdf: hgversion
a4book.pdf: hgversion

hgversion:
	hg id | sed 's/_/\\_/g' > hgversion.tex
	date --date="`hg log -l 1 --template='{date|isodate}' .`" +'%d %B %Y' > hgdate.tex

.SUFFIXES: .tex .pdf

.tex.pdf:
	pdflatex $< && pdflatex $<

g: letter.pdf
	evince letter.pdf

k: letter.pdf
	okular letter.pdf

clean:
	rm -f *.aux *.log *.pdf *.toc \
	    hgversion.tex hgdate.tex

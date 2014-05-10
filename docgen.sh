#! /bin/bash

doxygen lab3.cfg
cd latex
pdflatex refman.tex
cp refman.pdf ../lab3codedoc.pdf
cd ..
rm -r html 
rm -r latex

#htmldoc --webpage -f lab3docs.pdf ./html/*.html


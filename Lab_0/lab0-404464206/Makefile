SHELL := /bin/bash
.SILENT:

default:
	gcc -g lab0.c -o lab0

check: default inputCheck outputCheck in-n-out noFile noReadInput noWriteOutput catchCheck everythingCheck
	if [ -s log.txt ]; \
	then \
		echo "Some checks failed. See below."; \
		cat log.txt; \
	else \
		echo "All checks succeeded!"; \
	fi
	rm -f log.txt; \

inputCheck:
	echo "input testing" > inputFile.txt; \
	echo | ./lab0 --input=inputFile.txt > outputFile.txt; \
	cmp inputFile.txt outputFile.txt >/dev/null 2>&1; \
	if [ $$? -ne 0 ]; \
	then \
		echo "inputCheck test failed!" >> log.txt; \
	fi; \
	rm -f inputFile.txt outputFile.txt

outputCheck:
	echo "output testing" > inputFile.txt; \
	cat inputFile.txt | ./lab0 --output=outputFile.txt; \
	cmp inputFile.txt outputFile.txt >/dev/null 2>&1; \
	if [ $$? -ne 0 ]; \
	then \
		echo "outputCheck test failed!" >> log.txt; \
	fi; \
	rm -f inputFile.txt outputFile.txt

in-n-out:
	echo "My name is Vamsi" > inputFile.txt; \
	echo "I like turtles" > outputFile.txt; \
	echo | ./lab0 --input=inputFile.txt --output=outputFile.txt; \
	cmp inputFile.txt outputFile.txt >/dev/null 2>&1; \
	if [ $$? -ne 0 ]; \
	then \
		echo "Combined --input + --output test failed!" >> log.txt; \
	fi; \
	rm -f inputFile.txt outputFile.txt

noFile:
	echo | ./lab0 >/dev/null 2>&1; \
	if [ $$? -ne 0 ]; \
	then \
		echo "No file test failed!" >> log.txt; \
	fi; \

noReadInput:
	touch inputFile.txt; \
	chmod u-r inputFile.txt; \
	echo | ./lab0 --input=inputFile.txt >/dev/null 2>&1; \
	if [ $$? -ne 1 ]; \
	then \
		echo "noReadInput test failed!" >> log.txt; \
	fi; \
	rm -f inputFile.txt

noWriteOutput:
	touch outputFile.txt; \
	chmod u-w outputFile.txt; \
	echo | ./lab0 --output=outputFile.txt >/dev/null 2>&1; \
	if [ $$? -ne 2 ]; \
	then \
		echo "noWriteOutput test failed!" >> log.txt; \
	fi; \
	rm -f outputFile.txt

catchCheck:
	echo | ./lab0 --catch --segfault >/dev/null 2>&1; \
	if [ $$? -ne 3 ]; \
	then \
		echo "catchCheck test failed!" >> log.txt; \
	fi; \

everythingCheck:
	echo "final testing" > inputFile.txt; \
	echo "almost done" > outputFile.txt; \
	echo | ./lab0 --input=inputFile.txt --output=outputFile.txt --segfault --catch >/dev/null 2>&1; \
	if [ $$? -ne 3 ]; \
	then \
		echo "everythingCheck test failed!" >> log.txt; \
	fi; \
	rm -f inputFile.txt outputFile.txt

clean:
	rm -f *.o lab0 *.txt *~ 

dist:
	tar -czvf lab0-404464206.tar.gz lab0.c Makefile *.png README

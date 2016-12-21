.SILENT:

default:
	gcc -lmraa -lm -g lab4a.c -o lab4a
	gcc -lmraa -lm -g -pthread lab4b.c -o lab4b

log_4a:
	gcc -lmraa -lm -g lab4a.c -o lab4a
	./lab4a > log_4a.txt

log_4b:
	gcc -lmraa -lm -g -pthread lab4b.c -o lab4b
	./lab4b > log_4b.txt

clean:
	rm -f lab4a lab4b *~

dist:
	tar -zcvf lab4-404464206.tar.gz lab4a.c lab4b.c log_4a.txt log_4b.txt Makefile

.SILENT: 

build: default tests profile graphs tarball

default: lab2_add lab2_list

lab2_add:
	rm -f lab2_add
	gcc -g -Wall -pthread -lrt lab2_add.c -o lab2_add

lab2_list:
	rm -f lab2_list
	gcc -g -pthread -lrt SortedList.c lab2_list.c -o lab2_list

tests:
	cp /dev/null lab_2b_list.csv

	# generate time per operation numbers
	./lab2_add --threads=1 --iterations=10000 --sync=m >> lab_2b_list.csv
	./lab2_add --threads=2 --iterations=10000 --sync=m >> lab_2b_list.csv
	./lab2_add --threads=4 --iterations=10000 --sync=m >> lab_2b_list.csv
	./lab2_add --threads=8 --iterations=10000 --sync=m >> lab_2b_list.csv
	./lab2_add --threads=12 --iterations=10000 --sync=m >> lab_2b_list.csv
	./lab2_add --threads=1 --iterations=10000 --sync=s >> lab_2b_list.csv
	./lab2_add --threads=2 --iterations=10000 --sync=s >> lab_2b_list.csv
	./lab2_add --threads=4 --iterations=10000 --sync=s >> lab_2b_list.csv
	./lab2_add --threads=8 --iterations=10000 --sync=s >> lab_2b_list.csv
	./lab2_add --threads=12 --iterations=10000 --sync=s >> lab_2b_list.csv

	# measure the performance
	./lab2_list --threads=1 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=s >> lab_2b_list.csv

	# No sync lists test
	-./lab2_list --threads=1 --iterations=1 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=1 --iterations=2 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=1 --iterations=4 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=1 --iterations=8 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=1 --iterations=16 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=4 --iterations=1 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=4 --iterations=2 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=4 --iterations=4 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=4 --iterations=8 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=4 --iterations=16 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=8 --iterations=1 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=8 --iterations=2 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=8 --iterations=4 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=8 --iterations=8 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=8 --iterations=16 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=12 --iterations=1 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=12 --iterations=2 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=12 --iterations=4 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=12 --iterations=8 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=12 --iterations=16 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=16 --iterations=1 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=16 --iterations=2 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=16 --iterations=4 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=16 --iterations=8 --yield=id --lists=4 >> lab_2b_list.csv
	-./lab2_list --threads=16 --iterations=16 --yield=id --lists=4 >> lab_2b_list.csv

	# With sync lists test
	./lab2_list --threads=1 --iterations=10 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=1	--iterations=20	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=1	--iterations=40	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=1	--iterations=80	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4	--iterations=10	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4	--iterations=20	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4	--iterations=40	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4	--iterations=80	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8	--iterations=10	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8	--iterations=20	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8	--iterations=40	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8	--iterations=80	--yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=10 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=20 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=40 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=80 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=10 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=20 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=40 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=80 --yield=id --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=1	--iterations=10	--yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=1 --iterations=20 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=1 --iterations=40 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=1 --iterations=80 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=10 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=20 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=40 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=80 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=10 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=20 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=40 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=80 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=10 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=20 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=40 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=80 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=10 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=20 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=40 --yield=id --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=16 --iterations=80 --yield=id --lists=4 --sync=m >> lab_2b_list.csv

	# lab2b_4.png: Mutex synchronized throughput
	./lab2_list --threads=1	--iterations=1000 --lists=4 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=1	--iterations=1000 --lists=8 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=1	--iterations=1000 --lists=16 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=2	--iterations=1000 --lists=4 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=2	--iterations=1000 --lists=8 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=2	--iterations=1000 --lists=16 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=4	--iterations=1000 --lists=4 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=4	--iterations=1000 --lists=8 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=4	--iterations=1000 --lists=16 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=8	--iterations=1000 --lists=4 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=8	--iterations=1000 --lists=8 --sync=m >>	lab_2b_list.csv
	./lab2_list --threads=8	--iterations=1000 --lists=16 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=4 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=8 --sync=m >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=16 --sync=m >> lab_2b_list.csv

	# lab2b_5.png: Spin-lock synchronized throughput
	./lab2_list --threads=1 --iterations=1000 --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=8 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=16 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=8 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=16 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=8 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=16 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=8 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=16 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=4 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=8 --sync=s >> lab_2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=16 --sync=s >> lab_2b_list.csv


profile:
	LD_PRELOAD=/u/cs/ugrad/vamsi/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=threadFunc ./lab2_list ./raw.gperf >> profile.gperf
	rm -f ./raw.gperf

graphs: lab_2b_graphs

lab_2b_graphs:
	/u/cs/grad/zhou/iloveos/gnuplot lab_2b_list.gp

clean:
	rm -f lab2_add lab2_list *~ *.png *.csv *.tar.gz *.o *.gperf

tarball:
	tar -zcvf lab2b-404464206.tar.gz lab2_add.c SortedList.h SortedList.c lab2_list.c Makefile *.csv *.gp *.gperf *.png README.txt

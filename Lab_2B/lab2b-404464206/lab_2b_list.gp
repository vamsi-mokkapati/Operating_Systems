#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

set title "Scalability-1: Synchronized Throughput"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'add-m,' lab_2b_list.csv" using ($2):(1000000000/($6)) \
          title 'adds w/mutex' with linespoints lc rgb 'blue', \
     "< grep 'add-s,' lab_2b_list.csv" using ($2):(1000000000/($6)) \
          title 'adds w/spin' with linespoints lc rgb 'orange', \
     "< grep 'List-none-m,.*,.*,1,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
          title 'list ins/lookup/delete w/mutex' with linespoints lc rgb 'grey', \
     "< grep 'List-none-s,.*,.*,1,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
          title 'list ins/lookup/delete w/spin' with linespoints lc rgb 'violet'

set title "Scalability-2: Per-operation Times for List Operations"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "mean time/operation (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top

plot \
     "< grep 'List-none-m,.*,.*,1,' lab_2b_list.csv" using ($2):($7) \
     title 'completion time' with linespoints lc rgb 'orange', \
     "< grep 'List-none-m,.*,.*,1,' lab_2b_list.csv" using ($2):($8) \
     title 'wait for lock' with linespoints lc rgb 'red'

set title "Scalability-3: Correct Synchronization of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Iterations"
set logscale y 10
set output 'lab2b_3.png'
set key left top

plot \
     "< grep 'List-id-none,' lab_2b_list.csv" using ($2):($3) \
     title 'yield=id' with points lc rgb 'red', \
     "< grep 'List-id-s,' lab_2b_list.csv" using ($2):($3) \
     title 'Spin-Lock' with points lc rgb 'green', \
     "< grep 'List-id-m,' lab_2b_list.csv" using ($2):($3) \
     title 'Mutex' with points lc rgb 'blue'

set title "Scalability-4: Mutex-Synchronized Throughput of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_4.png'
set key left top

plot \
     "< grep 'List-none-m,.*,.*,1,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=1' with linespoints lc rgb 'violet', \
     "< grep 'List-none-m,.*,.*,4,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=4' with linespoints lc rgb 'green', \
     "<	grep 'List-none-m,.*,.*,8,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=8' with linespoints lc rgb 'blue', \
     "<	grep 'List-none-m,.*,.*,16,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=16' with linespoints lc rgb 'orange'

set title "Scalability-5: Spin-Lock-Synchronized Throughput of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_5.png'
set key left top

plot \
     "< grep 'List-none-s,.*,.*,1,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=1' with linespoints lc rgb 'violet', \
     "<	grep 'List-none-s,.*,.*,4,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=4' with linespoints lc rgb 'green', \
     "< grep 'List-none-s,.*,.*,8,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'List-none-s,.*,.*,16,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=16' with linespoints lc rgb 'orange'
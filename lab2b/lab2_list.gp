#! /usr/bin/gnuplot
#NAME: Qinglin Zhang
#EMAIL: qqinglin0327@gmail.com
#ID: 205356739
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

#1 Total number of operations per second for each synchronization
set title "Total number of operations per second for each synchronization"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput(operation per sec)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w spin' with linespoints lc rgb 'green'


#2 wait-for-lock time, and the average time per operation against the number of competing threads
set title "mean time per mutex wait and mean time per operation for mutex-synchronized list operations"
set xlabel "Number of Threads"
set logscale x 2
set ylabel "Time"
set logscale y 10
set output 'lab2b_2.png'

plot \
    "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
        title 'wait for lock time' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
        title 'average time per operation' with linespoints lc rgb 'green'

#3 sublist runs with synchronization vs w/o synchronization
set title "mean time per mutex wait and mean time per operation for mutex-synchronized list operations"
set xlabel "Threads"
set logscale x 2
set ylabel "Successful Iterations"
set logscale y 10
set xrange [0.5:]
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3)\
    with points lc rgb "red" title "Unprotected", \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    with points lc rgb "blue" title "Mutex", \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    with points lc rgb "green" title "Spin-Lock"

#4 the aggregated throughput (total operations per second) 
set title "hroughput vs. number of threads w mutex"
set xlabel "Threads"
set ylabel "Throughput"
set logscale x 2
set logscale y 10
set xrange [0.5:]
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	    title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	    title 'lists=4' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	    title 'lists=8' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv"using ($2):(1000000000/($7)) \
	    title 'lists=16' with linespoints lc rgb 'orange'

# the aggregated throughput (total operations per second) vs. the number of threads w spin
set title "hroughput vs. number of threads w spin-lock"
set xlabel "Threads"
set ylabel "Throughput"
set logscale x 2
set logscale y 10
set xrange [0.5:]
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
            title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
            title 'lists=4' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
            title 'lists=8' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv"using ($2):(1000000000/($7)) \
            title 'lists=16' with linespoints lc rgb 'orange'
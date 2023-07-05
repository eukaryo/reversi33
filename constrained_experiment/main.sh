#!/bin/sh
git clone https://github.com/arminbiere/kissat.git -b sc2022-bulky
cd kissat
./configure && make test
cd ../
make
./dimacs-cnf-maker 33 &
pid1=$!
./dimacs-cnf-maker 34 &
pid2=$!
./dimacs-cnf-maker 33black &
pid3=$!
wait $pid1
wait $pid2
wait $pid3
./kissat/build/kissat dimacs_cnf_33_cpp.txt --sat | tee bulky-cnf33-log.txt &
pid1=$!
./kissat/build/kissat dimacs_cnf_34_cpp.txt --unsat | tee bulky-cnf34-log.txt &
pid2=$!
./kissat/build/kissat dimacs_cnf_33_black_cpp.txt --sat | tee bulky-cnf33-black-log.txt &
pid3=$!
wait $pid1
wait $pid2
wait $pid3
python3 interpret-log.py | tee scores.txt
#!/bin/sh
python3 -m venv venv-z3-solver-unconstrained
. venv-z3-solver-unconstrained/bin/activate
pip install z3-solver==4.12.2
python -u unconstrained.py | tee unconstrained-experiment-log.txt
deactivate
rm -rf venv-z3-solver-unconstrained

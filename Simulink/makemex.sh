#!/bin/bash

${MATLAB_ROOT}/bin/mex -I../include -g PDAIRead.c -lpowerdaq32
${MATLAB_ROOT}/bin/mex -I../include -g PDAOWrite.c -lpowerdaq32


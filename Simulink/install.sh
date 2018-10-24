#!/bin/bash

# Installation of PowerDAQ library  for Simulink
# Copyright United Electronics Industries, Inc. 2005

ID=`id -u`
if [ ${ID} != "0" ] ; then
	echo "You must be root to install, exiting."
	exit -1
fi

echo -n "Enter the root path of your MATLAB installation: "
read MATLAB_ROOT
if [ ! -d ${MATLAB_ROOT} ] ; then
	echo "Invalid MATLAB location, exiting."
	exit -1
fi
export MATLAB_ROOT

./makemex.sh

if [ -d ${MATLAB_ROOT}/toolbox/uei ] ; then
   rm -rf ${MATLAB_ROOT}/toolbox/uei
fi

mkdir ${MATLAB_ROOT}/toolbox/uei
cp -rf * ${MATLAB_ROOT}/toolbox/uei

echo "Configuring MATLAB/Simulink, please wait..."
pushd ${MATLAB_ROOT}/toolbox/uei 2>&1 > /dev/null

${MATLAB_ROOT}/bin/matlab -nodesktop -nosplash -r "addpath ${MATLAB_ROOT}/toolbox/uei;savepath;exit" 2>&1 > /dev/null

popd 2>&1 > /dev/null 

echo " Installation complete"



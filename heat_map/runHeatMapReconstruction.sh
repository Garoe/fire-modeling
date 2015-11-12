#!/bin/bash
if [ "$#" -le 0 ]; then
	echo ""
	echo "Missing solver type"
	echo ""
	echo "Usage: runHeatMapReconstruction.sh <solver>"
	echo ""
	echo "	Where <solver> can be any of [\"ga\", \"sa\"]"
	echo "	\"ga\" -> Genetic Algorithm"
	echo "	\"sa\" -> Simulated Annealing"
	echo ""
	exit 0 
fi

# Create random name for the log file to avoid clashes with other matlab logs
LOGFILE=`mktemp matlabXXXXXXXXXXXXXXXXXXXXX.log`
# Runs matlab in batch mode
matlab -nodesktop -nosplash -r "heatMapSearch('$1', '$LOGFILE')" -logfile $LOGFILE

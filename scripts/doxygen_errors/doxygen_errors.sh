#!/bin/bash

# Generate the Doxygen configuration file
doxygen -g doxygen_errors.yml


# Check if doxygen generated any warnings
results=$?

if [ $results -eq 0 ];
   then
	# If doxygen does not have any warnings, code exits with zero status
	echo "Doxygen documentation succesfully generated"
	exit 0
elif [ $results -ne 0 ];
   then
	# If doxygen generated any warnings, code exits with non - zero status
	echo "Doxygen generated warnings!"
	exit 1
fi

# Run Doxygen
doxygen doxygen_errors.yml

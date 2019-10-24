#!/bin/bash

cd "$(dirname "$0")"
cd ..

SRC_FILES=$(ls R/*.R)
HEADER_FILES=$(find R/projectGenerator -name "*.R")
EXPECTED_FIRST_LINE="#  Rkernel is an execution kernel for R interpreter"

for file in $SRC_FILES $HEADER_FILES; do
  read -r FIRST_LINE < $file
  if [ "$EXPECTED_FIRST_LINE" == "$FIRST_LINE" ]; then
     echo "$file has license"
  else 
     echo "adding license to $file"
     cp license/preambule-gpl-3.0-R tmp.txt
     cat $file >> tmp.txt
     mv tmp.txt $file
  fi     
done

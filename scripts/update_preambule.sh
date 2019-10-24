#!/bin/bash

cd "$(dirname "$0")"
cd ..

SRC_FILES=$(find src -name "*.cpp")
HEADER_FILES=$(find src -name "*.h")
EXPECTED_FIRST_LINE="//  Rkernel is an execution kernel for R interpreter"

for file in $SRC_FILES $HEADER_FILES; do
  read -r FIRST_LINE < $file
  if [ "$EXPECTED_FIRST_LINE" == "$FIRST_LINE" ]; then
     echo "$file has license"
  else 
     echo "adding license to $file"
     cp license/preambule-gpl-3.0 tmp.txt
     cat $file >> tmp.txt
     mv tmp.txt $file
  fi     
done

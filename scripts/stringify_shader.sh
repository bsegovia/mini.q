#!/bin/bash
cat $1 |\
  # append comma at the beginning and at the end of each line
  sed 's/^/\"/' | sed 's/$/\\n\"/' |\
  # replace each '\' by '\\'
  sed 's/\\\\n\"/\\\\\\n\"/g' |\
  # remove comma for lines marked by '//##' (this is c++ code)
  sed 's/\"\/\/##\(.*\)\\n\"/\1/' |\
  # remove empty string lines
  sed 's/\"[\ \t]*\\n\"//g'

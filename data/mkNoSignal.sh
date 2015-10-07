#!/bin/bash

for inFile in noSignal4x3*.png ; do
  outFile="$(basename "$inFile" .png).mpg"
  cat "$inFile" \
  | pngtopnm \
  | ppmtoy4m -F 25:1 -A 4:3 -I p -r -S 420mpeg2 -v 2 -n 1 \
  | mpeg2enc -f 8 -a 2 -q 1 -n p -I 0 -T 120 -R 2 -g 10 -G 12 -o /dev/stdout \
  | cat >"$outFile"
done

for inFile in noSignal16x9*.png ; do
  outFile="$(basename "$inFile" .png).mpg"
  cat "$inFile" \
  | pngtopnm \
  | ppmtoy4m -F 25:1 -A 16:9 -I p -r -S 420mpeg2 -v 2 -n 1 \
  | mpeg2enc -f 8 -a 3 -q 1 -n p -I 0 -T 120 -R 2 -g 10 -G 12 -o /dev/stdout \
  | cat >"$outFile"
done


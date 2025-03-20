#!/usr/bin/env bash

files=$(ls ./*.png)
for file in $files; do
  if [[ $file =~ .*~color\.png$ ]]; then
    newfile="${file%~color.png}~bw.png"
    ffmpeg -i "$file" -filter_complex "[0]split=2[bg][fg];[bg]drawbox=c=black@1:replace=1:t=fill[bg];[bg][fg]overlay=format=auto[merged];[merged]format=gray,lutyuv=y=if(gte(val\,1)\,255\,0)" -pix_fmt monob "$newfile" -y
  fi
done

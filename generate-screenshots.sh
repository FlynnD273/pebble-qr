#!/usr/bin/env bash

rebble build

if [[ ! -d screenshots ]]; then
  mkdir screenshots
fi

function scr() {
  rebble install --emulator "$1"
  sleep 1
  rebble screenshot ./screenshots/"$1".png
  rebble kill
}

scr aplite
scr basalt
scr chalk
scr diorite

#!/usr/bin/env sh 

./build/midid \
  -o "USB|oscroll" \
  -i "Through" \
  -E "donk: 0.05s1.0 -> 0.2s0.5 -> SUST -> 0.6s0.0" \
  -O "osc1: wave=sin vol=1 bias=0.9 base=0" \
  -O "osc2: wave=sin vol=1.0  base=55" \
  -I "CoolBass donk: osc1 % osc2" \
  -C "0: CoolBass"

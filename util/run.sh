#!/usr/bin/env sh 

./build/midid \
  -o "USB|oscroll|Firefox" \
  -i "Through" \
  -E "donk: 0.05s1.0 -> 0.2s0.5 -> SUST -> 0.6s0.0" \
  -O "osc1: wave=triangle vol=0.5 bias=0.9 base=0" \
  -O "osc2: wave=sin vol=0.1" \
  -I "CoolBass donk: osc1 % osc2" \
  -C "0: CoolBass"

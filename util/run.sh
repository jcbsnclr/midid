#!/usr/bin/env sh

./build/midid                                         \
  -i Through                                          \
  -o 'USB|oscroll|Firefox'                            \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=0 wave=square base=0 vol=0.15 bias=0.9'    \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=1 wave=sin base=0 vol=0.2'                 \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=2 wave=triangle base=0 vol=0.2'            \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=3 wave=square bias=0.0 base=0 vol=0.15'    \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=4 wave=sin bias=0.0 base=12 vol=0.05'      \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=5 wave=triangle bias=0.0 base=24 vol=0.15' \
                                                      \
  -I '0.05s1.0 -> 0.1s0.35 -> SUST -> 0.5s0.0 
      chan=6 wave=square bias=0.0 base=0 vol=0.15'    \
                                                      \
  -I '0.01s1.0 -> SUST -> 0.1s0.0 
      chan=7 wave=square bias=0.99212 base=-12 vol=0.15'     \


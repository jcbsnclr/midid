#!/usr/bin/env sh 

./build/midid                                                                                                    \
  -o "USB|oscroll|Firefox|audacity" \
  -i "Through" \
  -E "donk: 0.02s1.0 -> 0.2s0.35 -> SUST -> 0.1s0.0" \
  -O "a: wave=sin vol=1.0" \
  -O "b: wave=sin vol=1.0 base=24" \
  -I "Lol donk: a - b" \
  -C "0: Lol" \
  -O "s: wave=triangle vol=1.0 bias=0.2" \
  -O "x: wave=triangle vol=1.0 bias=0.5" \
  -O "r: wave=square vol=1.0 bias=0.2" \
  -O "v: wave=sin vol=1.0 bias=0.2 hz=4" \
  -I "l donk: x - r * v" \
  -C "1: l" \
  -O "foo: wave=sin vol=1.0 bias=0.3" \
  -O "bar: wave=square vol=1.0 bias=0.3" \
  -I "Foo donk: foo % bar" \
  -C "2: Foo"

  
  # -C "3: l" \
  # -O "foo: wave=sin vol=1.0" \
  # -O "bar: wave=square vol=0.7 base=-24 bias=0.3" \
  # -O "baz: wave=triangle vol=1.0" \
  # -I "Lol donk: foo - bar" \
  # -C "0: Lol" \
  # -O "car: wave=sin vol=1.0" \
  # -O "mod: wave=sin vol=1.0 base=-24 bias=0.3" \
  # -I "True donk: car - mod" \
  # -O "bass: wave=triangle vol=1.0 base=-12" \
  # -O "bass_mod: wave=triangle vol=1.0 bias=0.5" \
  # -I "Bassy donk: bass - foo + bass_mod"\
  # -C "1: True"\
  # -C "2: Bassy" \

  
  # -O "o1: wave=sin vol=1.0 bias=0.1" \
  # -O "o2: wave=triangle vol=1.0 bias=0.3 base=-12" \
  # -O "o3: wave=sin vol=1.0 bias=0.1 base=-24" \
  # -I "Organ donk: o1 * o2 * o3"\
  # -C "3: Organ"

  # -O "foo: wave=sin vol=1.0 bias=0.3" \
  # -I "Foo donk: foo" \
  # -C "0: Foo"

  # -E "donk: 0.02s1.0 -> 0.2s0.35 -> SUST -> 0.1s0.0"   `# define an envelope (how note volume changes over time)` \
  #                                                                                                                \
  # -O "meaty1: wave=sin vol=1.0 bias=0.3"           `# define an oscillator`                                   \
  # -O "meaty2: wave=square vol=1.0  base=0 bias=0.7"                                                              \
  # -I "Meaty donk: meaty1 % meaty2"                    `# define an instrument `                                  \
  #                                                                                                                \
  # -O "lol1: wave=saw base=-24 vol=1.0"                                                                           \
  # -O "lol2: wave=saw base=0 vol=1.0"                                                                             \
  # \
  # \
  # -O "car: wave=sin vol=1.0" \
  # -O "mod: wave=sin vol=0.5 base=5" \
  # -I "True donk: car - mod" \
  # -I "LolWut donk: lol1 - meaty1"                                                                                   \
  #                                                                                                                \
  # -C "0: True"                                       `# assign instruments to channel `                         
    

{ pkgs ? import <nixpkgs> {}}:

pkgs.mkShell rec {
  nativeBuildInputs = with pkgs; [
    pkg-config
  ];
  
  buildInputs = with pkgs; [
    tokei
    pkg-config
    gcc
    libjack2
    clang-tools
    gnumake
    linenoise
    lldb
    bear
    gdb
  ];

  LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
}

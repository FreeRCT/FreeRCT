#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  HOMEBREW_NO_AUTO_UPDATE=1 brew install sdl2 sdl2_ttf
else
  # No SDL2 in the default repos yet
  sudo apt-add-repository ppa:zoogie/sdl2-snapshots -y
  # GCC 4.8 not in the repos either
  sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
  sudo apt-get -qq update
  sudo apt-get install -qq g++-4.8 # Needed for newer stdlib
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 50
  sudo apt-get -qq install cmake build-essential libsdl2-dev libsdl2-ttf-dev bison flex libpng-dev
fi

## TODO: in future, provide other options for LevelDB
mkdir build
g++ main.cpp -o build/dumpsteamcollections -std=c++20 -lleveldb -O2
strip build/dumpsteamcollections

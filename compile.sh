#!/bin/bash

printf "\n Compiling -> Token Contract \n \n"
cd eosio.token
# eosio-cpp -I ./include/ -I ../xyz/include/ -abigen -o=abc.wasm ./src/abc.cpp
# eosio-cpp -I ./include/ -abigen -o=eosio.token.wasm ./src/eosio.token.cpp
printf "\n Done \n"
cd ..

printf "\n Compiling -> Hytale Reputation \n \n"
cd hytale.reputation
eosio-cpp -I ./include/ -I ../eosio.token/include/ -abigen -o=hytale.reputation.wasm ./src/hytale.reputation.cpp
printf "\n Done \n"
cd ..
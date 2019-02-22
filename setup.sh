#!/bin/bash

USER_PUB_KEY=EOS7i1PgEe399sjbhhS6umNFU6okzit96chj8NtpBRzy6XpDYXUH9

function start() {
   0_create_accounts
   1_set_contracts
   2_run_setup
   3_register_accounts
}

function 0_create_accounts() {
   cleos system newaccount ultra hytaletokens ${USER_PUB_KEY} --transfer --stake-net "5000.0000 UOS" --stake-cpu "5000.0000 UOS" --buy-ram-kbytes 400  -p ultra --ultra-id 8
	cleos system newaccount ultra hytalecontra ${USER_PUB_KEY} --transfer --stake-net "5000.0000 UOS" --stake-cpu "5000.0000 UOS" --buy-ram-kbytes 400  -p ultra --ultra-id 9
}

function 1_set_contracts() {
   cleos set contract hytaletokens ./eosio.token -p hytaletokens
   cleos set contract hytalecontra ./hytale.reputation -p hytalecontra
}

function 2_run_setup() {
   cleos set account permission hytalecontra active --add-code
   cleos set account permission hytalecontra active --add-code hytaletokens owner -p hytalecontra
   cleos set account permission hytaletokens active --add-code hytalecontra owner -p hytaletokens
   cleos push action hytalecontra init '[]' -p hytalecontra
}

function 3_register_accounts() {
   cleos push action hytalecontra reguser '["usera","Registered new user."]' -p usera
   cleos push action hytalecontra reguser '["userb","Registered new user."]' -p userb
   cleos push action hytalecontra reguser '["ultra","Registered new user."]' -p ultra
}

start
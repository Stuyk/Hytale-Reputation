#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/singleton.hpp>
#include <eosio.token/eosio.token.hpp>

namespace stuyk_eos {
   using namespace std;
   using namespace eosio;

   // Properties
   const name     token_contract_name = name("hytaletokens");          // Contract we are interacting with.
   const symbol   hrep_symbol = symbol(symbol_code("HYTALE"), 4);        // Default Symbol
   // Staking
   const uint64_t seconds_in_a_day = 86400;
   const uint64_t unstake_days = 3;
   // Assets
   const asset maximum_supply = asset(20000000'0000ull, hrep_symbol);
   const asset    default_mine_amount = asset(5'0000ull, hrep_symbol);
   
   struct [[eosio::table("state"), eosio::contract("hytale.reputation")]] state {
      uint8_t is_initialized = 0;

      EOSLIB_SERIALIZE(state, (is_initialized))
   };
   typedef eosio::singleton<name("state"), state> state_table_s;

   // Code, Scope, Table
   // (hytale.reputation, user, reputation)
   struct [[eosio::table("reputation"), eosio::contract("hytale.reputation")]] reputation {
      name              user;                 // The user that this reputation belongs to.
      time_point_sec    next_claim_time;      // The next time the user can claim tokens.
      asset             rep_left_for_day = default_mine_amount; // Reputation left for the next 24 hours.
      bool              is_server        = false;
      asset             staked_tokens    = asset(0'0000ull, hrep_symbol);

      uint64_t primary_key() const { return user.value; }
      EOSLIB_SERIALIZE(reputation, (user)(next_claim_time)(rep_left_for_day)(is_server)(staked_tokens))
   };
   typedef eosio::multi_index<name("reputation"), reputation> reputation_table;

   // Class Start
   class [[eosio::contract("hytale.reputation")]] hytale_reputation : public contract {
      public:
         using contract::contract;
         hytale_reputation(name s, name code, datastream<const char*> ds);

         [[eosio::action]]
         void init();

         // Register Account to Hytale Reputation
         [[eosio::action]]
         void reguser( name user, string memo );

         // Mine Hytale Reputation and Give it to Another Player
         [[eosio::action]]
         void mine( name user_from, name user_to, asset quantity, string memo );

         // Register as a server owner.
         [[eosio::action]]
         void regserver( name user, string memo );

         // Unregister yourself as a server owner.
         [[eosio::action]]
         void unregserver( name user, string memo );

         // Unstake an amount of tokens.
         [[eosio::action]]
         void unstake( name user, asset quantity, string memo );
   };
}
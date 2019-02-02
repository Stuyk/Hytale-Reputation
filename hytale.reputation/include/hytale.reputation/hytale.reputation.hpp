#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/singleton.hpp>
#include <eosio.token/eosio.token.hpp>

namespace stuyk_eos {
   using namespace std;
   using namespace eosio;

   const name     token_contract_name = name("myhytalecash");          // Contract we are interacting with.
   const symbol   hrep_symbol = symbol(symbol_code("HREP"), 4);        // Default Symbol
   const asset    default_mine_amount = asset(5'0000ull, hrep_symbol); // 5.0000 unsigned long long
   const uint64_t seconds_in_a_day = 86400;
   const uint64_t unstake_days = 3;

   // Code, Scope, Table
   // (hytale.reputation, user, reputation)
   struct [[eosio::table("reputation"), eosio::contract("hytale.reputation")]] reputation {
      name              user;                 // The user that this reputation belongs to.
      time_point_sec    next_claim_time;      // The next time the user can claim tokens.
      uint64_t          rep_left_for_day = 5; // Reputation left for the next 24 hours.
      bool              is_server = false;
      asset             staked_tokens;
      time_point_sec    unstake_time;
      asset             unstake_amount;

      uint64_t primary_key() const { return user.value; }
      EOSLIB_SERIALIZE(reputation, (user)(next_claim_time)(rep_left_for_day)(is_server)(staked_tokens)(unstake_time)(unstake_amount))
   };
   typedef eosio::multi_index<name("reputation"), reputation> reputation_table;

   // Class Start
   class [[eosio::contract("hytale.reputation")]] hytale_reputation : public contract {
      public:
         using contract::contract;
         hytale_reputation(name s, name code, datastream<const char*> ds);

         // Register Account to Hytale Reputation
         [[eosio::action]]
         void reguser( name user, string memo );

         // Mine Hytale Reputation and Give it to Another Player
         [[eosio::action]]
         void mine( name user_from, name user_to, string memo );

         // Stake your tokens in exchange for Hytale Reputation.
         [[eosio::action]]
         void stake( name user, asset quantity, string memo );

         // Register as a server owner.
         [[eosio::action]]
         void regserver( name user, string memo );

         // Unregister yourself as a server owner.
         [[eosio::action]]
         void unregserver( name user, string memo );

         // Unstake an amount of tokens.
         [[eosio::action]]
         void unstake( name user, asset quantity, string memo );

         // Claim staked tokens after a set period of time. ie. 3 days.
         [[eosio::action]]
         void claim ( name user, string memo );
   };
}
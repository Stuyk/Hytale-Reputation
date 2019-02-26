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
   const name     token_contract_name = name("tokenshytale");          // Contract we are interacting with.
   const symbol   hrep_symbol = symbol(symbol_code("HYTALE"), 4);        // Default Symbol
   // Staking
   const uint64_t seconds_in_a_day = 86400;
   // Assets
   const asset maximum_supply = asset(20000000'0000ull, hrep_symbol);
   const asset    default_mine_amount = asset(5'0000ull, hrep_symbol);
   
   // Code, Scope, Table
   // (hytale.reputation, user, reputation)
   struct [[eosio::table("reputation"), eosio::contract("hytale.reputation")]] reputation {
      name                  user;                                                         // The user that this reputation belongs to.
      time_point_sec        next_claim_time   = time_point_sec(now() + seconds_in_a_day); // The next time the user can claim tokens.
      asset                 rep_left_for_day  = default_mine_amount;                      // Reputation left for the next 24 hours.
      bool                  is_server_owner   = false;                                    // Is a Server Owner?
      asset                 staked_tokens     = asset(0'0000ull, hrep_symbol);            // Total Tokens Staked from Trades
      uint64_t              successful_trades = 0;                                        // Number of successfuly trades.
      std::optional<string> avatar;                                                       // Imgur avatar a user can set.
      std::optional<string> server_address;

      uint64_t primary_key() const { return user.value; }
      EOSLIB_SERIALIZE(reputation, (user)(next_claim_time)(rep_left_for_day)
         (is_server_owner)(staked_tokens)(successful_trades)(avatar)(server_address))
   };
   typedef eosio::multi_index<name("reputation"), reputation> reputation_table;

   struct [[eosio::table("trades"), eosio::contract("hytale.reputation")]] trades {
      name                    owner;
      name                    reciever;
      asset                   cost;
      vector<uint8_t>         data;
      bool                    owner_sig;
      bool                    reciever_sig;
      string                  owner_hytale_name;
      string                  reciever_hytale_name;
      string                  conditions;

      uint64_t primary_key() const { return owner.value; }
      EOSLIB_SERIALIZE(trades, (owner)(reciever)(cost)(data)(owner_sig)(reciever_sig)(owner_hytale_name)(reciever_hytale_name)(conditions))
   };
   typedef eosio::multi_index<name("trades"), trades> trades_table;

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
         void regserver( name user, string server_address );

         // Unregister yourself as a server owner.
         [[eosio::action]]
         void unregserver( name user );

         // Unstake an amount of tokens.
         [[eosio::action]]
         void unstake( name user, asset quantity, string memo );

         // Set IMGUR Avatar
         [[eosio::action]]
         void setavatar( name user, string imgur_link );

         // Clear IMGUR Avatar
         [[eosio::action]]
         void clearavatar( name user );
   };
}
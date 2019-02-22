#include <hytale.reputation/hytale.reputation.hpp>

namespace stuyk_eos {
   hytale_reputation::hytale_reputation(name reciever, name code, datastream<const char*> ds) 
   : contract(reciever, code, ds) {}

   // Initializes token creation.
   void hytale_reputation::init() {
      require_auth(_self);

      state_table_s _state(_self, _self.value);
      auto state = _state.get_or_create(_self, {1});
      _state.set(state, _self);

      action(
         {{ _self, name("active") }, { token_contract_name, name("active")}},
         token_contract_name,
         name("create"),
         std::make_tuple(_self, maximum_supply) // issuer, maximum_supply
      ).send();
   }

   // Register a new user to recieve Hytale coins.
   void hytale_reputation::reguser( name user, string memo ) {
      require_auth(user);
      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_acc = rep_table.find(user.value);
      eosio_assert(user_acc == rep_table.end(), "account already exists");
      
      // Emplace new user.
      // User pays for the ram to place them in here.
      rep_table.emplace(user, [&](auto& rep) {
         rep.user = user;
         rep.next_claim_time = time_point_sec(now() + seconds_in_a_day);
      });
   }

   // Mine a new token and utilize the token contract to issue a new token.
   void hytale_reputation::mine( name from, name to, asset quantity, string memo ) {
      require_auth(from);
      require_recipient(from);
      require_recipient(to);
      
      // Symbol Check
      eosio_assert(quantity.is_valid(), "not a valid quantity");
      eosio_assert(quantity.symbol == hrep_symbol, "smybol mismatch");
      eosio_assert(quantity.amount != 0, "amount must be greater than zero");

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_from = rep_table.find(from.value);
      eosio_assert(user_from != rep_table.end(), "Issuer has not been registered yet.");

      auto user_to = rep_table.find(to.value);
      eosio_assert(user_to != rep_table.end(), "Reciever has not been registered yet.");
      eosio_assert(memo.size() <= 256, "memo is too big");

      bool new_claim_period = false;

      // If a new day has started for the user's mining period, refresh available amount.
      if (time_point_sec(now()) >= user_from->next_claim_time) {
         new_claim_period = true;
      }

      if (!new_claim_period) {
         eosio_assert(quantity.amount <= user_from->rep_left_for_day.amount, "mining period exceeded, wait 24 hours and try again");
      }

      // If a new claim period has been reached... reset the mine amount.
      // As well as subtract the quantity we need to subtract.
      rep_table.modify(user_from, from, [&](auto& rep) {
         if (new_claim_period) {
            rep.rep_left_for_day.amount = default_mine_amount.amount;
         }

         rep.rep_left_for_day.amount -= quantity.amount;
      });

      if (user_to->is_server) {
         action(
            {{ _self, name("active") }},
            token_contract_name,
            name("issue"),       // Action
            std::make_tuple(_self, quantity, std::string("Reputation Earned as Server Owner"))  // name to, asset quantity, string memo
         ).send();

         rep_table.modify(user_to, _self, [&](auto& rep) {
            rep.staked_tokens.amount = (quantity.amount + rep.staked_tokens.amount);
         });
      } else {
         // Not Staking
         // Send Action to Token Contract, Needs eosio.code permissions.
         // Issue a single token to the user we are mining to.
         action(
            {{ _self, name("active") }},
            token_contract_name, // 
            name("issue"),       // Action
            std::make_tuple(to, quantity, std::string("Reputation Earned"))  // name to, asset quantity, string memo
         ).send();
      }
   }

   void hytale_reputation::regserver( name user, string memo ) {
      require_auth(user);
      require_recipient(user);

      eosio_assert(memo.size() <= 256, "memo too large");

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_itr = rep_table.find(user.value);
      eosio_assert(user_itr != rep_table.end(), "Issuer has not been registered yet.");
      eosio_assert(!user_itr->is_server, "Already registered as a server.");

      rep_table.modify(user_itr, user, [&](auto& rep) {
         rep.is_server = true;
      });
   }

   void hytale_reputation::unregserver( name user, string memo ) {
      require_auth(user);
      require_recipient(user);

      eosio_assert(memo.size() <= 256, "memo too large");

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_itr = rep_table.find(user.value);
      eosio_assert(user_itr != rep_table.end(), "Issuer has not been registered yet.");
      eosio_assert(user_itr->is_server, "Already unregistered as a server.");

      rep_table.modify(user_itr, user, [&](auto& rep) {
         rep.is_server = false;
      });
   }

   void hytale_reputation::unstake( name user, asset quantity, string memo ) {
      require_auth(user);
      require_recipient(user);

      eosio_assert(memo.size() <= 256, "memo too large");
      eosio_assert(quantity.amount != 0, "amount must be greater than zero");
      eosio_assert(quantity.symbol == hrep_symbol, "symbol precision mismatch");

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_itr = rep_table.find(user.value);
      eosio_assert(user_itr != rep_table.end(), "user has not been registered yet.");
      eosio_assert(user_itr->staked_tokens.amount >= quantity.amount, "cannot reclaim an amount greater than available.");

      rep_table.modify(user_itr, user, [&](auto& rep) {
         rep.staked_tokens.amount -= quantity.amount;
      });

      action(
         permission_level{ _self, name("active") },
         token_contract_name, // 
         name("transfer"),    // Action
         std::make_tuple(_self, user, quantity, std::string("Tokens have been unstaked. They cannot be re-staked."))  // name to, asset quantity, string memo
      ).send();
   }
}

// Action Definitions
EOSIO_DISPATCH( stuyk_eos::hytale_reputation, (reguser)(mine)(regserver)(unregserver)(unstake)(init) )
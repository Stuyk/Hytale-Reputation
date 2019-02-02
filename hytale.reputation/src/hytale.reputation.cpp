#include <hytale.reputation/hytale.reputation.hpp>

namespace stuyk_eos {
   hytale_reputation::hytale_reputation(name reciever, name code, datastream<const char*> ds) 
   : contract(reciever, code, ds) {}

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
   void hytale_reputation::mine( name from, name to, string memo ) {
      require_auth(from);
      require_recipient(from);
      require_recipient(to);

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_from = rep_table.find(from.value);
      eosio_assert(user_from != rep_table.end(), "Issuer has not been registered yet.");

      auto user_to = rep_table.find(to.value);
      eosio_assert(user_to != rep_table.end(), "Reciever has not been registered yet.");

      eosio_assert(memo.size() <= 256, "memo is too big");

      bool new_claim = false;
      // Try claiming new
      if (user_from->rep_left_for_day == 0) {
         eosio_assert(time_point_sec(now()) >= user_from->next_claim_time, "user new claim time is not up.");
         new_claim = true;
      }

      rep_table.modify(user_from, from, [&](auto& rep) {
         if (new_claim) {
            rep.rep_left_for_day = (default_mine_amount.amount - 1);
         } else {
            rep.rep_left_for_day -= 1;
         }
      });

      // Send Action to Token Contract, Needs eosio.code permissions.
      // Issue a single token to the user we are mining to.
      action(
         permission_level{ _self, name("active") },
         token_contract_name, // 
         name("issue"),       // Action
         std::make_tuple(to, asset(1'0000ull, hrep_symbol), "Mined a new token.")  // name to, asset quantity, string memo
      ).send();
   }

   void hytale_reputation::stake( name user, asset quantity, string memo ) {
      require_auth(user);
      require_recipient(user);

      eosio_assert(memo.size() <= 256, "memo too large");

      eosio_assert(quantity.amount != 0, "amount must be greater than zero");
      asset actual_amount = asset(quantity.amount, hrep_symbol); // Re-Classify Currency

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_itr = rep_table.find(user.value);
      eosio_assert(user_itr != rep_table.end(), "User has not been registered yet.");
      eosio_assert(user_itr->is_server, "User is not registered as a server owner.");

      rep_table.modify(user_itr, user, [&](auto& rep) {
         rep.staked_tokens.amount += actual_amount.amount;
      });

      // Action
      action(
         permission_level{ user, name("active") },
         token_contract_name, // 
         name("transfer"),    // Action
         std::make_tuple(user, token_contract_name, actual_amount, "Staked tokens for reputation.")  // name to, asset quantity, string memo
      ).send();
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
      asset actual_amount = asset(quantity.amount, hrep_symbol); // Re-Classify Currency

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_itr = rep_table.find(user.value);
      eosio_assert(user_itr != rep_table.end(), "User has not been registered yet.");
      eosio_assert(user_itr->is_server, "User is not registered as a server owner.");
      eosio_assert(user_itr->staked_tokens.amount >= actual_amount.amount, "Cannot reclaim an amount greater than available.");

      rep_table.modify(user_itr, user, [&](auto& rep) {
         rep.staked_tokens.amount -= actual_amount.amount;
         rep.unstake_time          = time_point_sec(now() + (seconds_in_a_day * unstake_days)); // Days
         rep.unstake_amount       += actual_amount;
      });
   }

   void hytale_reputation::claim( name user, string memo ) {
      require_auth(user);
      require_recipient(user);
      eosio_assert(memo.size() <= 256, "memo too large");

      reputation_table rep_table = reputation_table(_self, _self.value);
      auto user_itr = rep_table.find(user.value);
      eosio_assert(user_itr != rep_table.end(), "User has not been registered yet.");
      eosio_assert(user_itr->unstake_amount.amount > 0, "Nothing to reclaim.");
      eosio_assert(user_itr->unstake_time <= time_point_sec(now()), "Your tokens are still being unstaked. Try again in a day or two.");

      asset amount_to_recieve = user_itr->unstake_amount;

      rep_table.modify(user_itr, user, [&](auto& rep) {
         rep.unstake_amount.amount = 0;
      });

      // Action
      action(
         permission_level{ user, name("active") },
         token_contract_name, // 
         name("transfer"),    // Action
         std::make_tuple(token_contract_name, user, amount_to_recieve, "Unstaked tokens.")  // name to, asset quantity, string memo
      ).send();
   }
}

// Action Definitions
EOSIO_DISPATCH( stuyk_eos::hytale_reputation, (reguser)(mine)(stake)(regserver)(unregserver)(unstake)(claim) )
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

#include <string>

#include "config.h"

#define STR_EXPAND(C) #C
#define STR(C) STR_EXPAND(C)

namespace eosiosystem {
	class system_contract;
}

namespace eosio {
	using std::string;

	class token : public contract {
	public:
		token(account_name self);

		void create(account_name issuer, asset maximum_supply);
		void issue(account_name to, asset quantity, string memo);
		void transfer(account_name from, account_name to, asset quantity, string memo);
		void allowclaim(account_name from, asset quantity);
		void claim(account_name from, account_name to, asset quantity);

		inline asset get_supply(symbol_name sym) const;
		inline asset get_balance(account_name owner, symbol_name sym) const;

	private:
		struct account {
			asset balance;
			int64_t blocked;
			uint64_t primary_key() const { return balance.symbol.name(); }
		};

		struct currency_stats {
			asset supply;
			asset max_supply;
			account_name issuer;
			uint64_t primary_key() const { return supply.symbol.name(); }
		};

		struct claim_t {
			account_name to;
			asset quantity;
			uint128_t primary_key() const { return ((uint128_t)to << 64) + quantity.symbol; }
		};

		typedef eosio::multi_index<N(accounts), account> accounts;
		typedef eosio::multi_index<N(stat), currency_stats> stats;
		typedef eosio::multi_index<N(claim), claim_t> claims;

		account_name exchange;

		void sub_balance(account_name owner, asset value, account_name ram_payer);
		void add_balance(account_name owner, asset value, account_name ram_payer);

	public:
		struct transfer_args {
			account_name from;
			account_name to;
			asset quantity;
			string memo;
		};
	};

	asset token::get_supply(symbol_name sym) const {
		stats statstable(this->_self, sym);
		const auto& st = statstable.get(sym);
		return st.supply;
	}

	asset token::get_balance(account_name owner, symbol_name sym) const {
		accounts accountstable(this->_self, owner);
		const auto& ac = accountstable.get(sym);
		asset balance = ac.balance;
		balance.amount -= ac.blocked;
		return balance;
	}
} /// namespace eosio

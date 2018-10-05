#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

#include <string>

class wutoken : public eosio::contract {
public:
	wutoken(account_name self);
	~wutoken();

	void create(account_name issuer, eosio::asset maximum_supply);
	void issue(account_name to, eosio::asset quantity, std::string memo);
	void transfer(account_name from, account_name to, eosio::asset quantity, std::string memo);
	void allowclaim(account_name from, account_name to, eosio::asset quantity);
	void claim(account_name from, account_name to, eosio::asset quantity);
	void cleanstate(eosio::vector<eosio::symbol_type> symbols, eosio::vector<account_name> accounts);

	inline eosio::asset get_supply(eosio::symbol_name sym) const;
	inline eosio::asset get_balance(account_name owner, eosio::symbol_name sym) const;

private:
	struct account {
		eosio::asset balance;
		int64_t blocked;
		uint64_t primary_key() const { return balance.symbol.name(); }
	};

	struct currency_stats {
		eosio::asset supply;
		eosio::asset max_supply;
		account_name issuer;
		uint64_t primary_key() const { return supply.symbol.name(); }
	};

	struct claim_t {
		uint64_t pk;
		account_name to;
		eosio::asset quantity;
		uint64_t primary_key() const { return pk; }
		uint128_t get_to_quantity() const { return ((uint128_t)to << 64) + quantity.symbol; }
	};

	typedef eosio::multi_index<N(accounts), account> accounts;
	typedef eosio::multi_index<N(stat), currency_stats> stats;
	typedef eosio::multi_index<N(claim), claim_t, eosio::indexed_by<N(toquantity), eosio::const_mem_fun<claim_t, uint128_t, &claim_t::get_to_quantity>>> claims;

	account_name exchange;

	void sub_balance(account_name owner, eosio::asset value, account_name ram_payer);
	void add_balance(account_name owner, eosio::asset value, account_name ram_payer);

public:
	struct transfer_args {
		account_name from;
		account_name to;
		eosio::asset quantity;
		std::string memo;
	};
};

eosio::asset wutoken::get_supply(eosio::symbol_name sym) const {
	stats statstable(this->_self, sym);
	const auto& st = statstable.get(sym);
	return st.supply;
}

eosio::asset wutoken::get_balance(account_name owner, eosio::symbol_name sym) const {
	accounts accountstable(this->_self, owner);
	const auto& ac = accountstable.get(sym);
	eosio::asset balance = ac.balance;
	balance.amount -= ac.blocked;
	return balance;
}

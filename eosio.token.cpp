#include "eosio.token.hpp"

namespace eosio {

token::token(account_name self) :
	contract(self),
	state_singleton(this->_self, this->_self),
	state(state_singleton.exists() ? state_singleton.get() : default_parameters())
{}

token::~token() {
	this->state_singleton.set(this->state, this->_self);
}

void token::create(account_name issuer, asset maximum_supply) {
	require_auth(this->_self);

	auto sym = maximum_supply.symbol;
	eosio_assert(sym.is_valid(), "invalid symbol name");
	eosio_assert(maximum_supply.is_valid(), "invalid supply");
	eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");

	stats statstable(this->_self, sym.name());
	auto existing = statstable.find(sym.name());
	eosio_assert(existing == statstable.end(), "token with symbol already exists");

	statstable.emplace(this->_self, [&](auto& s) {
		s.supply.symbol = maximum_supply.symbol;
		s.max_supply	= maximum_supply;
		s.issuer = issuer;
	});
}

void token::issue(account_name to, asset quantity, string memo) {
	auto sym = quantity.symbol;
	eosio_assert(sym.is_valid(), "invalid symbol name");
	eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

	auto sym_name = sym.name();
	stats statstable(this->_self, sym_name);
	auto existing = statstable.find(sym_name);
	eosio_assert(existing != statstable.end(), "token with symbol does not exist, create token before issue");
	const auto& st = *existing;

	require_auth(st.issuer);
	eosio_assert(quantity.is_valid(), "invalid quantity");
	eosio_assert(quantity.amount > 0, "must issue positive quantity");

	eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
	eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

	statstable.modify(st, 0, [&](auto& s) {
		s.supply += quantity;
	});

	add_balance(st.issuer, quantity, st.issuer);

	if (to != st.issuer) {
		SEND_INLINE_ACTION(*this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo});
	}
}

void token::transfer(account_name from, account_name to, asset quantity, string memo) {
	eosio_assert(from != to, "cannot transfer to self");
	require_auth(from);
	eosio_assert(is_account(to), "to account does not exist");
	auto sym = quantity.symbol.name();
	stats statstable(this->_self, sym);
	const auto& st = statstable.get(sym);

	require_recipient(from);
	require_recipient(to);

	eosio_assert(quantity.is_valid(), "invalid quantity");
	eosio_assert(quantity.amount > 0, "must transfer positive quantity");
	eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
	eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

	sub_balance(from, quantity, from);
	add_balance(to, quantity, from);
}

void token::allowclaim(account_name from, account_name to, asset quantity) {
	require_auth(this->state.exchange);

	require_recipient(from);
	require_recipient(to);

	accounts from_acnts(this->_self, from);
	const auto& account = from_acnts.find(quantity.symbol.name());
	eosio_assert(account != from_acnts.end(), "symbol not found");
	from_acnts.modify(account, from, [quantity](auto& a) {
		a.blocked += quantity.amount;
	});

	claims from_claims(this->_self, from);
	const auto& claim = from_claims.find(((uint128_t)to << 64) + quantity.symbol);
	if (claim == from_claims.end()) {
		from_claims.emplace(from, [to, quantity](auto& c) {
			c.to = to;
			c.quantity = quantity;
		});
	} else {
		if (claim->quantity.amount == -quantity.amount) {
			from_claims.erase(claim);
		} else {
			from_claims.modify(claim, from, [quantity](auto& c) {
				c.quantity += quantity;
			});
		}
	}
}

void token::claim(account_name from, account_name to, asset quantity) {
	require_auth(this->state.exchange);

	eosio_assert(quantity.amount > 0, "claim must be positive");

	accounts from_acnts(this->_self, from);
	const auto& account = from_acnts.find(quantity.symbol.name());
	eosio_assert(account != from_acnts.end(), "symbol not found");
	from_acnts.modify(account, to, [quantity](auto& a) {
		a.blocked -= quantity.amount;
	});

	claims from_claims(this->_self, from);
	const auto& claim = from_claims.find(((uint128_t)to << 64) + quantity.symbol);
	eosio_assert(claim != from_claims.end(), "no available claim");
	if (claim->quantity.amount == quantity.amount) {
		from_claims.erase(claim);
	} else {
		from_claims.modify(claim, to, [quantity](auto& c) {
			c.quantity -= quantity;
		});
	}

	sub_balance(from, quantity, to);
	add_balance(to, quantity, to);
}

void token::sub_balance(account_name owner, asset value, account_name ram_payer) {
	accounts from_acnts(this->_self, owner);

	const auto& from = from_acnts.get(value.symbol.name(), "no balance object found");
	eosio_assert(from.balance.amount - from.blocked >= value.amount, "overdrawn balance");

	if (from.balance.amount == value.amount) {
		from_acnts.erase(from);
	} else {
		from_acnts.modify(from, ram_payer, [&](auto& a) {
			a.balance -= value;
		});
	}
}

void token::add_balance(account_name owner, asset value, account_name ram_payer) {
	accounts to_acnts(this->_self, owner);
	auto to = to_acnts.find(value.symbol.name());
	if (to == to_acnts.end()) {
		to_acnts.emplace(ram_payer, [&](auto& a) {
			a.balance = value;
			a.blocked = 0;
		});
	} else {
		to_acnts.modify(to, 0, [&](auto& a) {
			a.balance += value;
		});
	}
}

} /// namespace eosio

EOSIO_ABI(eosio::token, (create)(issue)(transfer)(allowclaim)(claim))

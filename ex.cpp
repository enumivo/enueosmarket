#include "ex.hpp"

#include <cmath>
#include <enulib/action.hpp>
#include <enulib/asset.hpp>
#include "enu.token.hpp"

using namespace enumivo;
using namespace std;

void ex::receivedenu(const currency::transfer &transfer) {
  if (transfer.to != _self) {
    return;
  }

  // get ENU balance
  double enu_balance = enumivo::token(N(enu.token)).
	   get_balance(N(enu.eos.mm), enumivo::symbol_type(ENU_SYMBOL).name()).amount;
  
  enu_balance = enu_balance/10000;

  double received = transfer.quantity.amount;
  received = received/10000;

  // get EOS balance
  double eos_balance = enumivo::token(N(stable.coin)).
	   get_balance(N(enu.eos.mm), enumivo::symbol_type(EOS_SYMBOL).name()).amount;

  eos_balance = eos_balance/10000;

  //deduct fee
  received = received * 0.997;
  
  double product = eos_balance * enu_balance;

  double buy = eos_balance - (product / (received + enu_balance));

  auto to = transfer.from;

  auto quantity = asset(10000*buy, EOS_SYMBOL);

  action(permission_level{N(enu.eos.mm), N(active)}, N(stable.coin), N(transfer),
         std::make_tuple(N(enu.eos.mm), to, quantity,
                         std::string("Buy EOS with ENU")))
      .send();

  action(permission_level{_self, N(active)}, N(enu.token), N(transfer),
         std::make_tuple(_self, N(enu.eos.mm), transfer.quantity,
                         std::string("Buy EOS with ENU")))
      .send();
}

void ex::receivedeos(const currency::transfer &transfer) {
  if (transfer.to != _self) {
    return;
  }

  // get EOS balance
  double eos_balance = enumivo::token(N(stable.coin)).
	   get_balance(N(enu.eos.mm), enumivo::symbol_type(EOS_SYMBOL).name()).amount;
  
  eos_balance = eos_balance/10000;

  double received = transfer.quantity.amount;
  received = received/10000;

  // get ENU balance
  double enu_balance = enumivo::token(N(enu.token)).
	   get_balance(N(enu.eos.mm), enumivo::symbol_type(ENU_SYMBOL).name()).amount;

  enu_balance = enu_balance/10000;

  //deduct fee
  received = received * 0.997;

  double product = enu_balance * eos_balance;

  double sell = enu_balance - (product / (received + eos_balance));

  auto to = transfer.from;

  auto quantity = asset(10000*sell, ENU_SYMBOL);

  action(permission_level{N(enu.eos.mm), N(active)}, N(enu.token), N(transfer),
         std::make_tuple(N(enu.eos.mm), to, quantity,
                         std::string("Sell EOS for ENU")))
      .send();

  action(permission_level{_self, N(active)}, N(stable.coin), N(transfer),
         std::make_tuple(_self, N(enu.eos.mm), transfer.quantity,
                         std::string("Sell EOS for ENU")))
      .send();
      
}

void ex::apply(account_name contract, action_name act) {

  if (contract == N(enu.token) && act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();

    enumivo_assert(transfer.quantity.symbol == ENU_SYMBOL,
                 "Must send ENU");
    receivedenu(transfer);
    return;
  }

  if (contract == N(stable.coin) && act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();

    enumivo_assert(transfer.quantity.symbol == EOS_SYMBOL,
                 "Must send EOS");
    receivedeos(transfer);
    return;
  }

  if (act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();
    enumivo_assert(false, "Must send EOS or ENU");
    return;
  }

  if (contract != _self) return;

}

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  ex enueos(receiver);
  enueos.apply(code, action);
  enumivo_exit(0);
}
}

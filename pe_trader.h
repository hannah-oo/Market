#ifndef PE_TRADER_H
#define PE_TRADER_H

#include "pe_common.h"

#define MAXI_QTY 1000

char *read_exchange();
int exec_order(char *order_inst);
void signal_handler(int signal, siginfo_t *info, void *context);

#endif
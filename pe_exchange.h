#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"

#define LOG_PREFIX "[PEX]"
#define PRODUCT_CHAR_LIMIT 18
#define ORDER_TYPE_LIMIT 8
#define SPACE " "

struct Trader
{
    int id;
    int exchange_fdw;
    int trader_fdr;
    int pid;
    int fee;
    int *pos_qty;
    long *pos_price;
    int connected;
    int cur_order_id;
};

struct Product
{
    char *name;
};

struct Order
{
    int order_id;
    int price;
    int qty;
    int time;
    int disconnected;
    char *order_type;
    char *product_name;
    struct Product *product;
    struct Trader *trader;
    struct Order *next;
};

extern struct Product *reading_file(char *argv1, int *total_product);
extern int process_order(char *cur_order, char **order_type, int *order_id, char **product_name, int *qty, int *price, struct Product *product_list, int total_product);
extern void signal_handler(int signal, siginfo_t *info, void *context);
extern int make_orderbook(struct Product *product_list, struct Trader *trader_list, int *total_product, int total_trader, int time, struct Order *head);
extern int buy(struct Order **new_order, struct Order **head, int time, int *total_product, struct Product *product_list, long int *total_fee, int amend);
extern int sell(struct Order **new_order, struct Order **head, int time, int *total_product, struct Product *product_list, long int *total_fee, int amend);
extern int insert_order(struct Order **head, struct Order **new_order);
extern int amend_order(struct Order **head, int time, int order_id, int qty, int price, int cur_pid,  int *total_product, struct Product *product_list, long int *total_fee,struct Trader *trader_list, int total_trader);
extern int cancel_order(struct Order **head, int order_id, int cur_pid, char **order_type, char **product_name);
extern int my_atoi(char *num_str);
#endif
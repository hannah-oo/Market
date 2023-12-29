#ifndef PE_EXCHANGE_H

#include "pe_exchange.h"
#include "orderbook_and_insert_node.c"

#endif

int amend_order(struct Order **head, int time, int order_id, int qty, int price, int cur_pid, int *total_product, struct Product *product_list, long int *total_fee, struct Trader *trader_list, int total_trader)
{
    struct Order *cur_node = *head;
    struct Order *arrange_node = NULL;
    struct Order *prev = NULL;
    int id_found = 0;

    while (cur_node != NULL)
    {
        if (cur_node->order_id == order_id && cur_node->trader->pid == cur_pid)
        {
            if (prev == NULL)
            {
                *head = cur_node->next;
            }
            else
            {
                prev->next = cur_node->next;
            }
            cur_node->qty = qty;
            cur_node->price = price;
            cur_node->time = time;
            arrange_node = cur_node;
            id_found = 1;
        }
        prev = cur_node;
        cur_node = cur_node->next;
    }

    if (id_found == 0)
        return 1;
    else
    {
        int amend = 1;
        insert_order(head, &arrange_node);

        char write_to_current_trader[EACH_LINE_TOTAL] = {0};
        char write_to_others[EACH_LINE_TOTAL] = {0};
        sprintf(write_to_current_trader, "AMENDED %d;", arrange_node->order_id);
        sprintf(write_to_others, "MARKET %s %s %d %d;", arrange_node->order_type, arrange_node->product_name, qty, price);

        for (int trader_index = 0; trader_index < total_trader; trader_index++)
        {
            if (trader_list[trader_index].pid == cur_pid)
            {
                write(trader_list[trader_index].exchange_fdw, write_to_current_trader, strlen(write_to_current_trader));
            }
            else
            {
                write(trader_list[trader_index].exchange_fdw, write_to_others, strlen(write_to_others));
            }
        }

        if (strcmp(arrange_node->order_type, "BUY"))
        { // if current node is buy order
            sell(&arrange_node, head, arrange_node->time, total_product, product_list, total_fee, amend);
        }
        else if (strcmp(arrange_node->order_type, "SELL"))
        {
            buy(&arrange_node, head, arrange_node->time, total_product, product_list, total_fee, amend);
        }
    }
    return 0;
}

int cancel_order(struct Order **head, int order_id, int cur_pid, char **order_type, char **product_name)
{
    struct Order *cur_node = *head;
    struct Order *prev = NULL;
    int id_found = 0;

    while (cur_node != NULL)
    {
        if (cur_node->order_id == order_id && cur_node->trader->pid == cur_pid)
        { // if the node is found, remove this node
            *product_name = malloc(sizeof(char) * PRODUCT_CHAR_LIMIT);
            strcpy(*product_name, cur_node->product_name);
            strcpy(*order_type, cur_node->order_type);
            if (prev == NULL)
            {
                *head = cur_node->next;
                free(cur_node->order_type);
                free(cur_node->product_name);
                free(cur_node);
                id_found = 1;
            }
            else
            {
                prev->next = cur_node->next;
                free(cur_node->order_type);
                free(cur_node->product_name);
                free(cur_node);
                id_found = 1;
            }
            break;
        }
        prev = cur_node;
        cur_node = cur_node->next;
    }

    if (id_found == 0)
        return 1;

    return 0;
}

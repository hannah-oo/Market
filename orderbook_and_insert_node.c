#ifndef PE_EXCHANGE_H

#include "pe_exchange.h"

#endif

int make_orderbook(struct Product *product_list, struct Trader *trader_list, int *total_product, int total_trader, int time, struct Order *head)
{
    printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);
    for (int each_product = 0; each_product < *total_product; each_product++)
    {
        struct Order *cur_node = head;
        int track_consec = 1;
        int temp_qty = 0;
        int buy_lev = 0;
        int sell_lev = 0;
        printf("%s\tProduct: %s; ", LOG_PREFIX, product_list[each_product].name);

        while (cur_node != NULL)
        {
            if (strcmp(product_list[each_product].name, cur_node->product->name) == 0)
            {
                if (cur_node->next != NULL && cur_node->price != cur_node->next->price)
                {
                    if (strcmp(cur_node->order_type, "BUY") == 0)
                    {
                        buy_lev++;
                    }
                    else if (strcmp(cur_node->order_type, "SELL") == 0)
                    {
                        sell_lev++;
                    }
                }
                else if (cur_node->next != NULL && cur_node->price == cur_node->next->price)
                {
                    if (strcmp(cur_node->next->product_name, product_list[each_product].name) != 0)
                    {
                        if (strcmp(cur_node->order_type, "BUY") == 0)
                        {
                            buy_lev++;
                        }
                        else if (strcmp(cur_node->order_type, "SELL") == 0)
                        {
                            sell_lev++;
                        }
                    }
                }
                else if (cur_node->next == NULL)
                {
                    if (strcmp(cur_node->order_type, "BUY") == 0)
                    {
                        buy_lev++;
                    }
                    else if (strcmp(cur_node->order_type, "SELL") == 0)
                    {
                        sell_lev++;
                    }
                }
            }
            cur_node = cur_node->next;
        }
        printf("Buy levels: %d; Sell levels: %d\n", buy_lev, sell_lev);

        cur_node = head;
        while (cur_node != NULL)
        {
            if (strcmp(product_list[each_product].name, cur_node->product->name) == 0)
            {
                // head node
                if (temp_qty == 0)
                {
                    temp_qty = cur_node->qty;
                }

                if (cur_node->next != NULL && cur_node->price == cur_node->next->price)
                {
                    if ((strcmp(cur_node->order_type, cur_node->next->order_type) == 0) && (strcmp(cur_node->product_name, cur_node->next->product_name) == 0))
                    {
                        // if cur node and next node have same price
                        // and if both cur node and next node have same order type
                        track_consec++;
                        temp_qty += cur_node->next->qty;
                    }
                    else
                    {
                        printf("%s\t\t%s ", LOG_PREFIX, cur_node->order_type);
                        if (track_consec > 1)
                            printf("%d @ $%d (%d orders)\n", temp_qty, cur_node->price, track_consec);
                        else
                            printf("%d @ $%d (%d order)\n", cur_node->qty, cur_node->price, track_consec);

                        track_consec = 1;
                        temp_qty = cur_node->next->qty;
                    }
                }
                else
                { // if there is no next null or no consec orders
                    printf("%s\t\t%s ", LOG_PREFIX, cur_node->order_type);
                    if (track_consec > 1)
                        printf("%d @ $%d (%d orders)\n", temp_qty, cur_node->price, track_consec);
                    else
                        printf("%d @ $%d (%d order)\n", cur_node->qty, cur_node->price, track_consec);

                    track_consec = 1;
                    if (cur_node->next != NULL)
                        temp_qty = cur_node->next->qty;
                    else
                        temp_qty = cur_node->qty;
                }
            }
            cur_node = cur_node->next;
        }
    }

    printf("%s\t--POSITIONS--\n", LOG_PREFIX);
    for (int each_trader = 0; each_trader < total_trader; each_trader++)
    {
        printf("%s\tTrader %d: ", LOG_PREFIX, each_trader);
        for (int each_product = 0; each_product < *total_product; each_product++)
        {
            printf("%s ", product_list[each_product].name);
            printf("%d ", trader_list[each_trader].pos_qty[each_product]);
            if (each_product == *total_product - 1)
            {
                printf("($%ld)\n", trader_list[each_trader].pos_price[each_product]);
            }
            else
            {
                printf("($%ld), ", trader_list[each_trader].pos_price[each_product]);
            }
        }
    }
    return 0;
}

int insert_order(struct Order **head, struct Order **new_order)
{
    struct Order *cur_node = *head;
    struct Order *prev = NULL;

    while (cur_node != NULL && cur_node->price >= (*new_order)->price)
    {
        prev = cur_node;
        cur_node = cur_node->next;
    }
    if (prev == NULL) // if inserting as the head
    {
        (*new_order)->next = *head;
        *head = *new_order;
    }
    else
    {
        prev->next = *new_order;
        (*new_order)->next = cur_node;
    }
    return 0;
}
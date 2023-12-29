#ifndef PE_EXCHANGE_H

#include "pe_exchange.h"
#include "orderbook_and_insert_node.c"

#endif

int buy(struct Order **new_order, struct Order **head, int time, int *total_product, struct Product *product_list, long int *total_fee, int amend)
{
	// when a buy order is recieved, compare it with each seller of same product
	// find the lowest sell price
	int done = 0;
	int avail_qty = (*new_order)->qty; // track how many buy qty is available

	while (done == 0) //	this will go on until there is no matching orders found
	{
		int eligible_sellers = 0;
		int oldest = 999999;
		int min_sell_price = 1000000;
		struct Order *cur_node = *head;
		struct Order *best_deal_node = NULL;

		while (cur_node != NULL)
		{
			//  check if it is a sell order with same product
			if (strcmp(cur_node->order_type, "SELL") == 0 && strcmp(cur_node->product->name, (*new_order)->product->name) == 0)
			{
				// if the buy order price is >=  sell order price
				if (cur_node->price <= (*new_order)->price)
				{
					eligible_sellers++;
					// if the minimum price found is more than cur_node price
					if (min_sell_price > cur_node->price)
					{
						// if the cur_node order came in later than min_time
						if (oldest > cur_node->time)
						{
							// get the minimum time (aka oldest buyer) & save the node pointer
							oldest = cur_node->time;
							min_sell_price = cur_node->price;
							best_deal_node = cur_node;
						}
					}
				}
			}
			cur_node = cur_node->next;
		}

		if (oldest == 999999 || min_sell_price == 1000000)
		{ // if there are no same product buy orders with higher price
			if (amend == 0)
				insert_order(head, new_order);
				
			return 0;
		}

		// find how many qty can be purchased
		int purchase = 0;
		while (avail_qty > 0 && purchase < best_deal_node->qty)
		{
			avail_qty--;
			purchase++;
		}

		long int cost = (long int)purchase * best_deal_node->price;
		long int fee = (long int)round(cost * 0.01);
		long int buyer_profit = 0;
		long int seller_profit = 0;
		*total_fee += fee;

		if ((*new_order)->time > best_deal_node->time)
		{
			buyer_profit = (-1) * (cost + fee);
			seller_profit = cost;
		}
		else
		{
			buyer_profit = (-1) * cost;
			seller_profit = cost + fee;
		}
		for (int i = 0; i < *total_product; i++)
		{
			if (strcmp(product_list[i].name, (*new_order)->product_name) == 0)
			{
				// new order is the seller so minus qty
				(*new_order)->trader->pos_qty[i] += purchase;
				(*new_order)->trader->pos_price[i] += buyer_profit;
				best_deal_node->trader->pos_qty[i] -= purchase;
				best_deal_node->trader->pos_price[i] += seller_profit;
			}
		}

		char write_to_seller[EACH_LINE_TOTAL] = {0};
		char write_to_buyer[EACH_LINE_TOTAL] = {0};

		sprintf(write_to_seller, "FILL %d %d;", best_deal_node->order_id, purchase);
		sprintf(write_to_buyer, "FILL %d %d;", (*new_order)->order_id, purchase);

		write((*new_order)->trader->exchange_fdw, write_to_buyer, strlen(write_to_buyer));
		write(best_deal_node->trader->exchange_fdw, write_to_seller, strlen(write_to_seller));

		printf("%s Match: Order %d ", LOG_PREFIX, best_deal_node->order_id);
		printf("[T%d], New Order %d ", best_deal_node->trader->id, (*new_order)->order_id);
		printf("[T%d], value: $%ld, ", (*new_order)->trader->id, cost);
		printf("fee: $%ld.\n", fee);

		// removing best_deal_node if quantity = 0
		if (best_deal_node->qty - purchase == 0)
		{
			cur_node = *head;
			if (*head == best_deal_node) // if head is the best deal node
			{
				struct Order *free_this = cur_node;
				*head = cur_node->next;
				free(free_this->order_type);
				free(free_this->product_name);
				free(free_this);
			}
			else
			{
				while (cur_node->next != NULL)
				{
					// remove cur_node->next
					if (cur_node->next == best_deal_node)
					{
						struct Order *free_this = cur_node->next;
						cur_node->next = best_deal_node->next;
						free(free_this->order_type);
						free(free_this->product_name);
						free(free_this);
						break;
					}
					cur_node = cur_node->next;
				}
			}
		}
		else
		{ // change the qty in cur_node
			cur_node = *head;
			while (cur_node != NULL)
			{
				if (cur_node == best_deal_node)
				{
					cur_node->qty = cur_node->qty - purchase;
					break;
				}
				cur_node = cur_node->next;
			}
		}

		eligible_sellers--;
		if (eligible_sellers == 0 || avail_qty == 0)
		{
			if (avail_qty > 0)
			{
				(*new_order)->qty = avail_qty;
				insert_order(head, new_order);
			}
			else if (avail_qty == 0)
			{	// remove new_order node if quantity == 0
				if (amend == 0) // if the node is not amended
				{
					free((*new_order)->order_type);
					free((*new_order)->product_name);
					free(*new_order);
					new_order = NULL;
				}
				else
				{ // if the node is amended, find the node in the linked list
					cur_node = *head;
					struct Order *prev = NULL;
					while (cur_node != NULL)
					{
						if (cur_node->order_id == (*new_order)->order_id && cur_node->trader->pid == (*new_order)->trader->pid)
						{
							if (prev == NULL)
							{
								*head = cur_node->next;
							}
							else
							{
								prev->next = cur_node->next;
							}
							free(cur_node->order_type);
							free(cur_node->product_name);
							free(cur_node);
							break;
						}
						prev = cur_node;
						cur_node = cur_node->next;
					}
				}
			}
			done = 1;
		}
	}
	return 0;
}

int sell(struct Order **new_order, struct Order **head, int time, int *total_product, struct Product *product_list, long int *total_fee, int amend)
{
	// when a sell order is recieved, compare it with each buyer of same product
	// find the highest buy price
	int done = 0;
	int avail_qty = (*new_order)->qty; // track how many sell qty is available

	while (done == 0) //	this will go on until there is no matching orders found
	{
		int eligible_buyers = 0;
		int oldest = 999999;
		int max_buy_price = -1;
		struct Order *cur_node = *head;
		struct Order *best_deal_node = NULL;

		while (cur_node != NULL)
		{
			//  check if it is a buy order with same product
			if (strcmp(cur_node->order_type, "BUY") == 0 && strcmp(cur_node->product->name, (*new_order)->product->name) == 0)
			{
				// if the new_sell order price is <=  buy order price
				if (cur_node->price >= (*new_order)->price)
				{
					eligible_buyers++;
					// if the maximum price found is less than cur_node price
					if (max_buy_price < cur_node->price)
					{
						// if the cur_node order came in later than min_time
						if (oldest > cur_node->time)
						{
							// get the minimum time (aka oldest buyer) & save the node pointer
							oldest = cur_node->time;
							max_buy_price = cur_node->price;
							best_deal_node = cur_node;
						}
					}
				}
			}
			cur_node = cur_node->next;
		}

		if (oldest == 999999 || max_buy_price == -1)
		{ // if there are no same product buy orders with higher price
			if (amend == 0)
				insert_order(head, new_order);

			return 0;
		}

		// find how many qty can be purchased
		int purchase = 0;
		while (avail_qty > 0 && purchase < best_deal_node->qty)
		{
			avail_qty--;
			purchase++;
		}

		long int cost = (long int)purchase * best_deal_node->price;
		long int fee = (long int)round(cost * 0.01);
		long int buyer_profit = 0;
		long int seller_profit = 0;
		*total_fee += fee;
		// setting up for orderbook
		if ((*new_order)->time > best_deal_node->time)
		{
			buyer_profit = (-1) * cost;
			seller_profit = (cost - fee);
		}
		else
		{
			buyer_profit = (-1) * (cost - fee);
			seller_profit = cost;
		}

		for (int i = 0; i < *total_product; i++)
		{
			if (strcmp(product_list[i].name, (*new_order)->product_name) == 0)
			{   // new order is the seller so minus qty
				(*new_order)->trader->pos_qty[i] -= purchase;
				(*new_order)->trader->pos_price[i] += seller_profit;
				best_deal_node->trader->pos_qty[i] += purchase;
				best_deal_node->trader->pos_price[i] += buyer_profit;
			}
		}

		char write_to_seller[EACH_LINE_TOTAL] = {0};
		char write_to_buyer[EACH_LINE_TOTAL] = {0};

		sprintf(write_to_buyer, "FILL %d %d;", best_deal_node->order_id, purchase);
		sprintf(write_to_seller, "FILL %d %d;", (*new_order)->order_id, purchase);

		write(best_deal_node->trader->exchange_fdw, write_to_buyer, strlen(write_to_buyer));
		write((*new_order)->trader->exchange_fdw, write_to_seller, strlen(write_to_seller));

		printf("%s Match: Order %d ", LOG_PREFIX, best_deal_node->order_id);
		printf("[T%d], New Order %d ", best_deal_node->trader->id, (*new_order)->order_id);
		printf("[T%d], value: $%ld, ", (*new_order)->trader->id, cost);
		printf("fee: $%ld.\n", fee);

		if (best_deal_node->qty - purchase == 0)
		{ // remove cur_node
			cur_node = *head;
			if (*head == best_deal_node) // if head is the best_deal_node
			{
				struct Order *free_this = cur_node;
				*head = cur_node->next;
				free(free_this->order_type);
				free(free_this->product_name);
				free(free_this);
			}
			else
			{
				while (cur_node->next != NULL)
				{
					// already know if head should be removed
					if (cur_node->next == best_deal_node)
					{
						struct Order *free_this = cur_node->next;
						cur_node->next = best_deal_node->next;
						free(free_this->order_type);
						free(free_this->product_name);
						free(free_this);
						break;
					}
					cur_node = cur_node->next;
				}
			}
		}
		else
		{ // change the qty in cur_node
			cur_node = *head;
			while (cur_node != NULL)
			{
				if (cur_node->order_id == best_deal_node->order_id && cur_node->trader->pid == best_deal_node->trader->pid)
				{
					cur_node->qty = cur_node->qty - purchase;
					break;
				}
				cur_node = cur_node->next;
			}
		}

		eligible_buyers -= 1;

		if (eligible_buyers == 0 || avail_qty == 0)
		{
			if (avail_qty > 0)
			{
				(*new_order)->qty = avail_qty;
				insert_order(head, new_order);
			}

			else if (avail_qty == 0)
			{
				if (amend == 0)
				{
					free((*new_order)->order_type);
					free((*new_order)->product_name);
					free(*new_order);
					new_order = NULL;
				}
				else
				{
					cur_node = *head;
					struct Order *prev = NULL;
					while (cur_node != NULL)
					{
						if (cur_node->order_id == (*new_order)->order_id && cur_node->trader->pid == (*new_order)->trader->pid)
						{
							if (prev == NULL)
							{
								*head = cur_node->next;
							}
							else
							{
								prev->next = cur_node->next;
							}
							free(cur_node->order_type);
							free(cur_node->product_name);
							free(cur_node);
							break;
						}
						prev = cur_node;
						cur_node = cur_node->next;
					}
				}
			}
			done = 1;
		}
	}
	return 0;
}

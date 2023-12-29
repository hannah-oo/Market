/**
 * comp2017 - assignment 3
 * Name : Pwint Shwe Yi Oo @ Hannah Oo
 * unikey: pwoo5799
 */

#ifndef PE_EXCHANGE_H

#include "pe_exchange.h"
#include "buy_and_sell.c"
#include "amend_and_cancel.c"
#include "orderbook_and_insert_node.c"
#include "read_file_and_pipe.c"

#endif

volatile sig_atomic_t read_this_trader_pid = -1;
volatile sig_atomic_t disconnect_this_trader_pid = -1;

#ifndef signal_handler
void signal_handler(int signal, siginfo_t *info, void *context)
{
	if (signal == SIGUSR1)
	{
		read_this_trader_pid = info->si_pid;
	}
	else if (signal == SIGCHLD)
	{
		disconnect_this_trader_pid = info->si_pid;
	}
}
#endif

int main(int argc, char **argv)
{
	int *total_product = malloc(sizeof(int));
	int disconnected_traders = 0;
	int total_trader = argc - 2; // arguments excluding program name and product file
	int time = 0;
	long int total_fee = 0;

	// register signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = &signal_handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);

	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		// Handle error
		printf("error");
	}

	printf("%s Starting\n", LOG_PREFIX);

	struct Product *product_list = reading_file(argv[1], total_product);

	struct Trader *trader_list = malloc(total_trader * sizeof(struct Trader));

	// set up each trader
	for (int trader_counter = 0; trader_counter < total_trader; trader_counter++)
	{
		char exchange_fifo[EACH_LINE_TOTAL];
		char trader_fifo[EACH_LINE_TOTAL];

		snprintf(exchange_fifo, EACH_LINE_TOTAL, FIFO_EXCHANGE, trader_counter);
		snprintf(trader_fifo, EACH_LINE_TOTAL, FIFO_TRADER, trader_counter);

		unlink(exchange_fifo); // first check if fifo has already been created
		unlink(trader_fifo);

		if (mkfifo(trader_fifo, FIFO_PERMISSION_NUM) != 0)
		{
			printf("Error making fifo\n");
			for (int i = 0; i < *total_product; i++)
				free(product_list[i].name);
			free(total_product);
			exit(1);
		}

		if (mkfifo(exchange_fifo, FIFO_PERMISSION_NUM) != 0)
		{
			printf("Error making fifo\n");
			for (int i = 0; i < *total_product; i++)
				free(product_list[i].name);
			free(total_product);
			exit(1);
		}

		printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_fifo);
		printf("%s Created FIFO %s\n", LOG_PREFIX, trader_fifo);
		printf("%s Starting trader %d (%s)\n", LOG_PREFIX, trader_counter, argv[2]);

		// inititalizing current trader
		trader_list[trader_counter].id = trader_counter;
		trader_list[trader_counter].fee = 0;
		trader_list[trader_counter].pos_price = malloc(*total_product * sizeof(long int));
		trader_list[trader_counter].pos_qty = malloc(*total_product * sizeof(long int));
		trader_list[trader_counter].connected = 1;
		trader_list[trader_counter].cur_order_id = -1;
		for (int product_index = 0; product_index < *total_product; product_index++)
		{
			trader_list[trader_counter].pos_price[product_index] = 0;
			trader_list[trader_counter].pos_qty[product_index] = 0;
		}

		trader_list[trader_counter].pid = fork();

		if (trader_list[trader_counter].pid == -1)
		{
			printf("Failed to fork");
			for (int i = 0; i < *total_product; i++)
				free(product_list[i].name);
			free(total_product);
			exit(1);
		}

		else if (trader_list[trader_counter].pid == 0) // child process running
		{
			char *trader_id_str = malloc(sizeof(int) * 4 + 1);
			sprintf(trader_id_str, "%d", trader_counter);
			if (execl(argv[trader_counter + 2], argv[trader_counter + 2], trader_id_str, (char *)0) == -1)
			{
				printf("Failed to replace the child process with trader binary");
				for (int i = 0; i < *total_product; i++)
					free(product_list[i].name);
				free(total_product);
				exit(1);
			}
			free(trader_id_str);
		}

		// opening pipes
		trader_list[trader_counter].exchange_fdw = open(exchange_fifo, O_WRONLY);
		printf("%s Connected to %s\n", LOG_PREFIX, exchange_fifo);

		trader_list[trader_counter].trader_fdr = open(trader_fifo, O_RDONLY);
		printf("%s Connected to %s\n", LOG_PREFIX, trader_fifo);
	}

	for (int trader_counter = 0; trader_counter < total_trader; trader_counter++)
	{ // send msg to every trader
		char *cur_msg = "MARKET OPEN;";
		write(trader_list[trader_counter].exchange_fdw, cur_msg, strlen(cur_msg));
	}

	for (int trader_counter = 0; trader_counter < total_trader; trader_counter++)
	{
		kill(trader_list[trader_counter].pid, SIGUSR1);
	}

	// create a head node
	struct Order *head = malloc(sizeof(struct Order));
	head->product_name = malloc(sizeof(char) * PRODUCT_CHAR_LIMIT);
	head->order_type = malloc(sizeof(char) * ORDER_TYPE_LIMIT);
	head->next = NULL;

	struct Order *current_order = NULL;

	while (1)
	{
		if (read_this_trader_pid == -1)
		{
			pause();
		}

		if (read_this_trader_pid != -1)
		{ // save current pid in another int variable before it is changed
			int current_pid = read_this_trader_pid;
			read_this_trader_pid = -1;
			struct Trader *current_trader = NULL;
			for (int trader_index = 0; trader_index < total_trader; trader_index++)
			{
				if (trader_list[trader_index].pid == current_pid)
				{
					current_trader = &trader_list[trader_index];
					break;
				}
			}

			char cur_order_line[EACH_LINE_TOTAL] = {0};
			int each_char = 0;
			int invalid_order = 0;

			while (read(current_trader->trader_fdr, &cur_order_line[each_char], 1) != EOF)
			{
				if (cur_order_line[each_char] == ';')
				{
					cur_order_line[each_char] = '\0';
					break;
				}
				each_char++;
			}

			printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, current_trader->id, cur_order_line);

			cur_order_line[each_char] = ' ';
			cur_order_line[each_char + 1] = '\0';

			// inititalize some variable
			char *order_type = malloc(sizeof(char) * EACH_LINE_TOTAL);
			int order_id = 0;
			int qty = 0;
			int price = 0;
			char *product_name = {0};
			int amend = 0;
			int cancel = 0;

			if (process_order(cur_order_line, &order_type, &order_id, &product_name, &qty, &price, product_list, *total_product) == -1)
			{
				invalid_order = 1;
			}

			if (strcmp(order_type, "AMEND") == 0)
				amend = 1;

			if (strcmp(order_type, "CANCEL") == 0)
				cancel = 1;

			if (invalid_order == 0 && (amend == 1 || cancel == 1))
			{
				if (time == 0) // no order to amend or cancel
				{
					invalid_order = 1;
				}
				else
				{
					if (strcmp(order_type, "AMEND") == 0)
						invalid_order = amend_order(&head, time, order_id, qty, price, current_pid, total_product, product_list, &total_fee, trader_list, total_trader);
					else if (strcmp(order_type, "CANCEL") == 0)
						invalid_order = cancel_order(&head, order_id, current_pid, &order_type, &product_name);
				}
			}

			if (invalid_order == 0 && order_id != (current_trader->cur_order_id + 1) && amend == 0 && cancel == 0)
			{ // for checking if order_id is valid
				invalid_order = 1;
			}

			if (time == 0 && invalid_order != 1 && amend == 0 && cancel == 0)
			{ // set up head node here for the first node
				head->order_id = order_id;
				head->price = price;
				head->qty = qty;
				head->time = time;
				strcpy(head->product_name, product_name);
				strcpy(head->order_type, order_type);
				for (int each_product = 0; each_product < *total_product; each_product++)
				{
					if (strcmp(product_list[each_product].name, product_name) == 0)
					{
						head->product = &product_list[each_product];
						break;
					}
				}
				for (int each_trader = 0; each_trader < total_trader; each_trader++)
				{
					if (trader_list[each_trader].id == current_trader->id)
					{
						head->trader = &trader_list[each_trader];
						break;
					}
				}
				head->next = NULL;
			}

			if (invalid_order != 1 && time != 0 && amend == 0 && cancel == 0)
			{ // if new order recieved has valid entries
				current_order = malloc(sizeof(struct Order));
				current_order->order_id = order_id;
				current_order->price = price;
				current_order->qty = qty;
				current_order->product_name = malloc(sizeof(char) * PRODUCT_CHAR_LIMIT);
				strcpy(current_order->product_name, product_name);
				current_order->order_type = malloc(sizeof(char) * ORDER_TYPE_LIMIT);
				strcpy(current_order->order_type, order_type);
				current_order->next = NULL;

				current_order->time = time;
				for (int each_product = 0; each_product < *total_product; each_product++)
				{
					if (strcmp(product_list[each_product].name, product_name) == 0)
					{
						current_order->product = &product_list[each_product];
						break;
					}
				}
				for (int each_trader = 0; each_trader < total_trader; each_trader++)
				{
					if (trader_list[each_trader].id == current_trader->id)
					{
						current_order->trader = &trader_list[each_trader];
						break;
					}
				}
			}

			char write_to_current_trader[EACH_LINE_TOTAL] = {0};
			char write_to_others[EACH_LINE_TOTAL] = {0};

			if (invalid_order == 1)
			{
				strcpy(write_to_current_trader, "INVALID;");
				write(current_trader->exchange_fdw, write_to_current_trader, strlen(write_to_current_trader));
			}
			else
			{
				if (cancel == 1)
				{
					sprintf(write_to_current_trader, "CANCELLED %d;", order_id);
					sprintf(write_to_others, "MARKET %s %s 0 0;", order_type, product_name);
					free(product_name);
				}
				else if (cancel == 0 && amend == 0)
				{
					sprintf(write_to_current_trader, "ACCEPTED %d;", order_id);
					current_trader->cur_order_id += 1;
					sprintf(write_to_others, "MARKET %s %s %d %d;", order_type, product_name, qty, price);
				}
			}

			if (invalid_order == 0 && amend == 0)
			{ // write a respective message to each trader
				for (int trader_index = 0; trader_index < total_trader; trader_index++)
				{
					if (trader_list[trader_index].pid == current_pid)
					{
						write(trader_list[trader_index].exchange_fdw, write_to_current_trader, strlen(write_to_current_trader));
					}
					else
					{
						write(trader_list[trader_index].exchange_fdw, write_to_others, strlen(write_to_others));
					}
				}
			}

			if (strcmp(order_type, "BUY") == 0 && time > 0 && invalid_order == 0)
			{
				if (amend == 0 && cancel == 0)
					invalid_order = buy(&current_order, &head, time, total_product, product_list, &total_fee, amend);
			}

			if (strcmp(order_type, "SELL") == 0 && time > 0 && invalid_order == 0)
			{
				if (amend == 0 && cancel == 0)
					invalid_order = sell(&current_order, &head, time, total_product, product_list, &total_fee, amend);
			}

			if (invalid_order == 0)
			{
				make_orderbook(product_list, trader_list, total_product, total_trader, time, head);
				time++;
			}

			for (int trader_index = 0; trader_index < total_trader; trader_index++)
			{ // send signal to all traders
				kill(trader_list[trader_index].pid, SIGUSR1);
			}

			free(order_type);
		}

		if (disconnect_this_trader_pid != -1)
		{
			for (int trader_index = 0; trader_index < total_trader; trader_index++)
			{
				if (trader_list[trader_index].pid == disconnect_this_trader_pid)
				{
					printf("%s Trader %d disconnected\n", LOG_PREFIX, trader_index);
					disconnected_traders++;
					trader_list[trader_index].connected = 0;
					close(trader_list[trader_index].exchange_fdw); // close pipes
					close(trader_list[trader_index].trader_fdr);
					disconnect_this_trader_pid = -1;
				}
			}
		}

		if (disconnected_traders == total_trader)
		{
			break;
		}
	}

	printf("%s Trading completed\n", LOG_PREFIX);
	printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, total_fee);

	for (int i = 0; i < total_trader; i++)
	{
		char exchange_fifo[EACH_LINE_TOTAL];
		char trader_fifo[EACH_LINE_TOTAL];

		snprintf(exchange_fifo, EACH_LINE_TOTAL, FIFO_EXCHANGE, i);
		snprintf(trader_fifo, EACH_LINE_TOTAL, FIFO_TRADER, i);

		unlink(exchange_fifo); // close fifo
		unlink(trader_fifo);

		free(trader_list[i].pos_price);
		free(trader_list[i].pos_qty);
	}

	for (int i = 0; i < *total_product; i++)
	{
		free(product_list[i].name);
	}

	struct Order *cur_node = head;

	while (cur_node != NULL)
	{
		if (cur_node->next == NULL)
		{
			free(cur_node->order_type);
			free(cur_node->product_name);
			free(cur_node);
			cur_node = NULL;
			break;
		}
		struct Order *next_node = cur_node->next;
		free(cur_node->order_type);
		free(cur_node->product_name);
		free(cur_node);
		cur_node = next_node;
	}

	free(trader_list);
	free(product_list);
	free(total_product);

	return 0;
}
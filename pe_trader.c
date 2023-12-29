#include "pe_trader.h"

int read_from_exchange_fd = 0;
int pe_trader_0 = 0;
int order_id = -1;
int accepted = -1;
char exchange_queue[EACH_LINE_TOTAL] = {0};
char trader_queue[EACH_LINE_TOTAL] = {0};
char received_message[EACH_LINE_TOTAL] = {0};

char *read_exchange()
{ // reading line by line: read until ';'
    read(read_from_exchange_fd, received_message, EACH_LINE_TOTAL);
    int i = 0;

    while (i < strlen(received_message) + 1)
    {
        if (received_message[i] == ';')
        {
            break;
        }
        i++;
    }
    received_message[i] = '\0';
    return received_message;
}

void signal_handler(int signal, siginfo_t *info, void *context)
{
    if (signal == SIGUSR1)
    {
        char *cur_inst = read_exchange();
        if (strcmp(cur_inst, "MARKET OPEN") == 0)
        {
            return;
        }
        else
        {
            if (exec_order(cur_inst) == -1)
            {
                exit(1);
            }
        }
    }
}

int exec_order(char *order_inst)
{
    accepted = 1;
    char *market = strtok(order_inst, " ");
    char *operation = strtok(NULL, " ");

    if ((strcmp(market, "MARKET") != 0) || (strcmp(operation, "SELL") != 0))
    {
        return 2; // if it doesn't start with "MARKET SELL"
    }
    char *product = strtok(NULL, " ");
    int quantity = atoi(strtok(NULL, " "));
    int price = atoi(strtok(NULL, " "));

    if (quantity >= MAXI_QTY)
    {
        return -1;
    }
    // make auto buy order
    char order[EACH_LINE_TOTAL];
    if (strcmp(operation, "SELL") == 0)
    {
        order_id += 1;
        snprintf(order, EACH_LINE_TOTAL, "BUY %d %s %d %d;", order_id, product, quantity, price);
    }
    // write to exchange and send signal
    write(pe_trader_0, order, strlen(order));
    accepted = 0;
    return 1;
}

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Not enough arguments\n");
        return 1;
    }

    // register signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = &signal_handler;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        // Handle error
        printf("error");
    }

    // connect to named pipes
    int trader_id = atoi(argv[1]);

    // pipe to read from exchange
    sprintf(exchange_queue, FIFO_EXCHANGE, trader_id);
    read_from_exchange_fd = open(exchange_queue, O_RDONLY);

    // pipe to write to exchange
    sprintf(trader_queue, FIFO_TRADER, trader_id);
    pe_trader_0 = open(trader_queue, O_WRONLY);

    // event loop:
    while (1)
    {
        sleep(1.80);
        if (accepted == 0)
            kill(getppid(), SIGUSR1);
    }

    close(read_from_exchange_fd);
    close(pe_trader_0);
}
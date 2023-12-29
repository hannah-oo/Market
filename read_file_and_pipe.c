#ifndef PE_EXCHANGE_H

#include "pe_exchange.h"

#endif

struct Product *reading_file(char *argv1, int *total_product)
{
	// open product file (in argv[1] place)
	// put the content in the product file into array of strings
	FILE *productfile = NULL;
	productfile = fopen(argv1, "r");
	char product_name[PRODUCT_CHAR_LIMIT] = {0};

	if (productfile == NULL)
	{
		printf("Product file cannot be opened\n");
		free(total_product);
		exit(1);
	}

	fgets(product_name, PRODUCT_CHAR_LIMIT, productfile);
	*total_product = atoi(product_name);

	struct Product *product_list = malloc(*total_product * sizeof(struct Product));

	for (int each_row = 0; each_row < *total_product; each_row++)
	{
		product_list[each_row].name = malloc(PRODUCT_CHAR_LIMIT * sizeof(char));
		fgets(product_name, PRODUCT_CHAR_LIMIT, productfile);
		product_name[strlen(product_name) - 1] = '\0';
		strcpy(product_list[each_row].name, product_name);
	}

	fclose(productfile);

	char tempchar = ' ';
	printf("%s Trading %d products: ", LOG_PREFIX, *total_product);
	for (int item = 0; item < *total_product; item++)
	{
		printf("%s", product_list[item].name);
		if (item == *total_product - 1)
		{
			tempchar = '\n';
			printf("%c", tempchar);
		}
		else
			printf("%c", tempchar);
	}

	return product_list;
}

int process_order(char *cur_order_line, char **order_type, int *order_id, char **product_name, int *qty, int *price, struct Product *product_list, int total_product)
{
	strcpy(*order_type, cur_order_line);

	*order_type = strtok(*order_type, SPACE);

	if (order_type == NULL)
		return -1;

	if (strcmp(*order_type, "BUY") == 0 || strcmp(*order_type, "SELL") == 0)
	{
		char *temp_id = strtok(NULL, SPACE);
		if (temp_id == NULL)
			return -1;
		else
		{
			*order_id = my_atoi(temp_id);
			if (*order_id > 999999 || *order_id < 0)
				return -1;
		}

		*product_name = strtok(NULL, SPACE);
		int product_found = 0;

		if (product_name == NULL || strlen(*product_name) > PRODUCT_CHAR_LIMIT)
			return -1;

		for (int i = 0; i < total_product; i++)
		{
			if (strcmp(*product_name, product_list[i].name) == 0)
			{
				product_found = 1;
			}
		}

		if (product_found == 0)
			return -1;

		// for qty
		char *temp_qty = strtok(NULL, SPACE);
		if (temp_qty == NULL)
			return -1;
		else
		{
			*qty = my_atoi(temp_qty);
			if (*qty > 999999 || *qty < 1)
				return -1;
		}

		// for price
		char *temp_price = strtok(NULL, SPACE);
		if (temp_price == NULL)
			return -1;
		else
		{
			*price = my_atoi(temp_price);
			if (*price > 999999 || *price < 1)
				return -1;
		}
	}

	else if (strcmp(*order_type, "AMEND") == 0)
	{
		char *temp_id = strtok(NULL, SPACE);
		if (temp_id == NULL)
			return -1;
		else
		{
			*order_id = my_atoi(temp_id);
			if (*order_id > 999999 || *order_id < 0)
				return -1;
		}

		char *temp_qty = strtok(NULL, SPACE);

		// for qty
		if (temp_qty == NULL)
			return -1;
		else
		{
			*qty = my_atoi(temp_qty);
			if (*qty > 999999 || *qty < 1)
				return -1;
		}

		// for price
		char *temp_price = strtok(NULL, SPACE);
		if (temp_price == NULL)
			return -1;
		else
		{
			*price = my_atoi(temp_price);
			if (*price > 999999 || *price < 1)
				return -1;
		}
	}

	else if (strcmp(*order_type, "CANCEL") == 0)
	{
		char *temp_id = strtok(NULL, SPACE);
		if (temp_id == NULL)
			return -1;
		else
		{
			*order_id = my_atoi(temp_id);
			if (*order_id > 999999 || *order_id < 0)
				return -1;
		}
	}

	else
	{ // if it is some other word for order_type
		return -1;
	}

	char *check_invalid = strtok(NULL, "");

	if (check_invalid != NULL)
	{
		return -1;
	}

	return 1;
}

int my_atoi(char *num_str)
{
	int temp = 0;
	int sign = 1;

	if (*num_str == '-')
	{
		sign = -1;
		num_str += 1;
	}

	while (*num_str != '\0')
	{
		// 48 = '0', 57 = '9'
		if (*num_str < 48 || *num_str > 57)
		{
			return -1;
		}
		temp = temp * 10 + (*num_str - 48);
		num_str += 1;
	}

	return temp * sign;
}

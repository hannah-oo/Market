1. Describing how the exchange works.
   
My exchange will start with mallocing memory for necessary objects like trader list and product list. 
It will read the command line arguments as per specs and make fifos accordingly and open pipes with 
traders replacing as exchange's children. I am using linked list for storing orders in a decending 
order according to the price listed in the order. For each order received, depending on the order and 
execution, my exchange will malloc an order node and connect to the list. The order is matched 
according to what will be the maximum buy price or minimum sell price depending on the order type.
 When the number of traders disconnected is the same as total traders, my while loop will come to an 
 end and do closing routines such as freeing memory, closing pipes, and unlinking fifos. 


2. Describing my design decisions for the trader.
   
My trader won't do anything untill it gets any message other than "MARKET OPEN;" from the pipe. Then,
 everytime a signal is received, it will read from exchange and if it starts with "MARKET SELL", my
 trader will respond with a "BUY" order. In order to be fault-tolerant, my trader will send a signal
  every 1.8 second after the market has opened and a message has been written to the exchange to make 
 sure the exchange notice my message in the pipe in the case of exchange not getting message from my
  trader.


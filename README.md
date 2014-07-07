Producer-Consumer
=================
This project is an implementation of the producer-consumer problem in C language. However, to make it a little more 
interesting than the traditional 1 producer-1 consumer, in this problem we have 1 producer and 2 consumers.
A few rules that were followed while coding this program were:
  1.Each token the producer generates is produced exactly one time.
  2.The first consumer thread multiples the token by 7 and inserts it into its queue and the second consumer
    thread by 5.
  3. Since the buffer size is limited, a token is removed from the buffer only after both the consumers have gotten it.
  4. A consumer can connect or disconnect dynamically from the stream.
  

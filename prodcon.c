 /* Lab5.c */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "queue_a.h"

#define BUFFER_SIZE 3
int idcnt = 1;
pthread_t s1, t1, t2, a1,b;
int count = 0;
pthread_t *conid1, *conid2;
typedef struct connectentry {
	struct connectentry *next;
	pthread_t *thread;
	int first;
} ConnectEntry;

typedef struct stream_struct {
	struct stream_struct *next;
	ConnectEntry *ce;       
	pthread_t *thread;      
	int last_pos;           
	int first_pos;          
	pthread_mutex_t mutex;  
	pthread_cond_t  put1;   
	pthread_cond_t  get1;  
	queue buffer;   
	void *args;     
	int id;         
} Stream;

typedef struct {
	Stream *self, *prod;
} Args;

Stream suc1, con1, con2, ask1;
Args suc1_args, con1_args, con2_args, ask1_args;
pthread_attr_t attr;
ConnectEntry *ce1,*ce2;

int get(void *stream, pthread_t *p, int x) 
{
	int ret;
	
	queue *q = &((Stream*)stream)->buffer;
	
	ConnectEntry *ce = ((Stream*)stream)->ce, *ptr,*current;

	pthread_mutex_t *mutex = &((Stream*)stream)->mutex;
	pthread_cond_t *put1 = &((Stream*)stream)->put1;
	pthread_cond_t *get1 = &((Stream*)stream)->get1;
	//printf("in get\n");

	if(ce == NULL) //(ce->first == 0 && ce->thread == 0) //ce == NULL that is
		return -3;
	
	if(x == 10){	
		for(ptr = ce; ptr!=NULL ; ptr=ptr->next){
			if(*ptr->thread == pthread_self())
				current = ptr;
			if(count == 0) {
				conid1 = current->thread;
				ce1 = current;
			}
			else if(count == 1){
				conid2 = current->thread;
				ce2 = current;
			}
			count++;
			}
		}
	else 
		current = ce;

	pthread_mutex_lock(mutex);

	if(x == 10){
		if(isEmpty(q) || current->first > ((Stream*)stream)->last_pos){
			//printf("GET : Consumer %d thread is waiting\n",((Stream*)stream)->id);fflush(stdout);
			pthread_cond_wait(get1, mutex);
		}
		ret = peek(q, current->first - ((Stream *)stream)->first_pos);
		current->first++;
		if(*ce->thread == pthread_self() && current->first <= ce->next->first || *ce->next->thread == pthread_self() && current->first <= ce->first){
		if(nelem(q)!=1){
			dequeue(q);
			((Stream*)stream)->first_pos++;
			pthread_cond_signal(put1);
			}
		}
	}
	else {
		ret = peek(q, current->first);
		dequeue(q);
		((Stream*)stream)->first_pos++;
		pthread_cond_signal(put1);
	}
	pthread_mutex_unlock(mutex);
	return ret;
	
}

/* 'value' is the value to move to the consumer */
void put(void *stream, int value)
{
	queue *q = &((Stream*)stream)->buffer;
	pthread_mutex_t *mutex = &((Stream*)stream)->mutex;
	pthread_cond_t *put1 = &((Stream*)stream)->put1;
	pthread_cond_t *get1 = &((Stream*)stream)->get1;

	pthread_mutex_lock(mutex);

	// If buffer is full then wait until buffer is emptied by 'get'
	//printf("nelem %d\n",nelem(q));
	if (nelem(q) >= BUFFER_SIZE) 
	{
		//printf("PUT : Thread waits as buffer is full\n");
		pthread_cond_wait(put1, mutex);
	}

	// Enqueue the token in buffer & increment last_pos
	enqueue(q, value);

	((Stream*)stream)->last_pos++;

	// Signal consumers in wait state due to 'buffer empty' that token is added in buffer
	pthread_cond_broadcast(get1);
	pthread_mutex_unlock(mutex);

	return;
}

/* Put 1,2,3,4,5... into the self stream */
void *producer (void *streams)
{
	//printf("producer is running\n");
	Stream *self = ((Args*)streams)->self;
	int i;
	for (i=1 ; ; i++)
	{
		put(self, i);
		//self->last_pos++;
		printf("Producer: sent %d, buf_sz=%d\n", i, nelem(&self->buffer));fflush(stdout);
		print(&self->buffer,"pro",self->id); fflush(stdout);
	}
	pthread_exit(NULL);
}

/* Final consumer in the network */
void *consumer (void *streams) 
{
	//printf("consumer is running\n");
	Stream *self = ((Args*)streams)->self;
	Stream *prod = ((Args*)streams)->prod;
	int in;
	int x = 10;
	while (true)
	{
		//printf("in consumer while\n");
		in = get(prod, &b, x);
		if (in > -1) 
		{
			in = (in)*(*(int*)(&self->args)); 
			put(self, in);
			print(&self->buffer, "cons", self->id); fflush(stdout);
		}
		else
			break;
	}
	pthread_exit(NULL);   
}


void *asker (void *streams)
{
	Stream *s1 = ((Args*)streams)->prod;
	Stream *s2 = (((Args*)streams)->prod)->next;
	char line[10000];
	int val, disconflag,conflag;
	while (true) 
	{
		sleep(1);
		printf("[1 or 2 or 3 or 4 or 0 to quit] >>> "); fflush(stdout);

		fgets(line, 9000, stdin);

		if (line[0] == '1')
		{
			val = get(s1, &a1,0);
			if (val >= 0) 
				printf("Asker: got %d from Consumer(%d)\n", val, s1->id);
			else if (val == -1) 
				printf("Buffer full and request not present\n");
			else if (val == -2)
				printf("Buffer empty\n");
			else
				printf("Connection does not exist\n");		
		} 
		else if (line[0] == '2') 
		{
			val = get(s2, &a1,0);
			if (val >= 0)
				printf("Asker: got %d from Consumer(%d)\n", val, s2->id);
			else if (val == -1)
				printf("Buffer full and request not present\n");
			else if (val == -2)
				printf("Buffer empty\n");
			else
				printf("Connection does not exist\n");
		}
		else if (line[0] == '3') 
		{
			printf("Which consumer do you want to disconnect? >>>");
			fgets(line, 9000, stdin);

			if(line[0] == '1')
			{
				disconflag = disconnect(s1);
				if (disconflag > 0)
					printf("Asker: Consumer disconnected successfully\n");
				else 
					printf("Asker: Consumer is already disconnected\n");
			}
			else if(line[0] == '2')
			{
				disconflag = disconnect(s2);
				if (disconflag > 0)
					printf("Asker: Consumer disconnected successfully\n");
				else 
					printf("Asker: Consumer is already disconnected\n");
			}
			else
			{
				printf("Asker: Invalid entry\n");
			}
		}
		else if (line[0] == '4') 
		{
			printf("Which consumer do you want to connect? >>>");
			fgets(line, 9000, stdin);

			if(line[0] == '1')
			{
				conflag = reconnect(s1,conid1,ce1);
				if (conflag > 0)
					printf("Asker: Consumer connected successfully\n");
				else 
					printf("Asker: Consumer is already connected\n");
			}
			else if(line[0] == '2')
			{
				conflag = reconnect(s2,conid2,ce2);
				if (conflag > 0)
					printf("Asker: Consumer connected successfully\n");
				else 
					printf("Asker: Consumer is already connected\n");
			}
			else
			{
				printf("Asker: Invalid entry\n");
			}
		}
		else if (line[0] == '0')
		{
			printf("Asker: have a good day!\n");
			break;
		}
		else
			printf("Asker: not valid\n");
	}

	pthread_exit(NULL);
}

// Diconnects consumer from producer
int disconnect(void *stream)
{	
	if(((Stream*)stream)->ce != NULL)
	{
		(((Stream*)stream)->ce)->first = 0;
		(((Stream*)stream)->ce)->thread = 0;
		((Stream*)stream)->ce = NULL;
		//printf("first is null\n");
		return 1;
	}
	else
		return 0;
}

// Connects consumer from producer
int reconnect(void *stream, pthread_t *conid, ConnectEntry *cc)
{
	if(((Stream*)stream)->ce == NULL){
		((Stream*)stream)->ce = cc;
		(((Stream*)stream)->ce)->first = suc1.first_pos;
		(((Stream*)stream)->ce)->thread = conid;
		return 1;
	}
	else
		return 0;
}

/* initialize streams - see also queue_a.h and queue_a.c */
void init_stream (Args *args, pthread_t *thread, Stream *self, void *data) 
{
	if (self != NULL) 
	{
		self->thread = thread;
		self->ce = NULL;
		self->next = NULL;
		self->args = data;
		self->id = idcnt++;
		self->last_pos = -1;
		self->first_pos = 0;
		init_queue(&self->buffer);
		pthread_mutex_init(&self->mutex, NULL);
		pthread_cond_init(&self->put1, NULL);
		pthread_cond_init(&self->get1, NULL);
	}
	args->self = self;
	args->prod = NULL;
}

/* free allocated space in the queue - see queue_a.h and queue_a.c */
void kill_stream(Stream *stream) 
{ 
	destroy_queue(&stream->buffer);    /* provided by queue_a.c */
	pthread_cancel(*stream->thread);
}

/* puts an initialized stream object onto the end of a stream's input list */
void connect (Args *arg, Stream *s) 
{  
	// Add s to the argument list of consumer
	s->next = arg->prod;
	arg->prod = s;

	// If s is a produce stream then
	ConnectEntry *ptr = s->ce;
	s->ce = (ConnectEntry*)malloc(sizeof(ConnectEntry));
	s->ce->next = ptr;	
	
	// Initialize consumer's data
	s->ce->first = 0;
	s->ce->thread = arg->self->thread;
}


int main ()
{
	init_stream(&suc1_args, &s1, &suc1, NULL);   

	init_stream(&con1_args, &t1, &con1, (void*)7);
	connect(&con1_args, &suc1);                   

	init_stream(&con2_args, &t2, &con2, (void*)5);
	connect(&con2_args, &suc1);                   

	init_stream(&ask1_args, &a1, &ask1, NULL);    
	connect(&ask1_args, &con1);                   
	connect(&ask1_args, &con2);  

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&s1, &attr, producer, (void*)&suc1_args);
	pthread_create(&t1, &attr, consumer, (void*)&con1_args);

	pthread_create(&t2, &attr, consumer, (void*)&con2_args);

	pthread_create(&a1, &attr, asker,    (void*)&ask1_args);
	pthread_create(&b, NULL, NULL, 0);
	pthread_join(a1, NULL);
	kill_stream(&suc1);
	kill_stream(&con1);
	kill_stream(&con2);
	kill_stream(&ask1);

	pthread_exit(NULL);
}

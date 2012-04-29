#include "zwm.h"

Client *head = NULL;
Client *tail = NULL;

void zwm_client_push_head(Client *c)
{
	c->next = head;
	c->prev = NULL;
	if (head) {
		head->prev = c;
	} else {
		tail = c;
	}
	head = c;
}

void zwm_client_remove(Client *c)
{
	if(c->prev){
		c->prev->next = c->next;
	} else {
		head = c->next;
	}

	if(c->next){
		c->next->prev = c->prev;
	} else {
		tail = c->prev;
	}
}

void zwm_client_push_tail(Client *c)
{
	c->prev = tail;
	c->next = NULL;
	if(tail){
		tail->next = c;
	} else {
		head = c;
	}
	tail = c;
}



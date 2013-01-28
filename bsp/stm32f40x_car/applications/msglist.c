/*
消息格式，包括收发的统一
 */
#include <stdlib.h>
#include "msglist.h"



MsgListNode* msglist_node_create(void* data)
{
	MsgListNode* node = malloc(sizeof(MsgListNode));

	if(node != NULL)
	{
		node->prev = NULL;
		node->next = NULL;
		node->sibling_dn=NULL;
		node->sibling_up=NULL;
		node->data = data;
	}

	return node;
}

void msglist_node_destroy(MsgListNode* node)
{
	if(node != NULL)
	{
		node->next = NULL;
		node->prev = NULL;
		node->sibling_dn=NULL;
		node->sibling_up=NULL;
		free(node);
	}

	return;
}

MsgList* msglist_create(void)
{
	MsgList* thiz = malloc(sizeof(MsgList));

	if(thiz != NULL)
	{
		thiz->first = NULL;
	}

	return thiz;
}

MsgListRet msglist_prepend(MsgList* thiz, void* data)
{
	MsgListNode* node = NULL;
	MsgListNode* iter=NULL;

	if((node = msglist_node_create(data)) == NULL)
	{
		return MSGLIST_RET_OOM; 
	}

	if(thiz->first == NULL)
	{
		thiz->first = node;
		return MSGLIST_RET_OK;
	}
	
	iter= thiz->first;
	while(iter->prev != NULL)
	{
		iter = iter->prev;
	}
	iter->prev=node;
	node->next=iter;
	thiz->first=node;
	return MSGLIST_RET_OK;

}


MsgListRet msglist_append(MsgList* thiz, void* data)
{
	MsgListNode* node = NULL;
	MsgListNode* iter=NULL;

	if((node = msglist_node_create(data)) == NULL)
	{
		return MSGLIST_RET_OOM; 
	}

	if(thiz->first == NULL)
	{
		thiz->first = node;
		return MSGLIST_RET_OK;
	}
	
	iter= thiz->first;
	while(iter->next != NULL)
	{
		iter = iter->next;
	}
	iter->next=node;
	node->prev=iter;
	return MSGLIST_RET_OK;
}





size_t   msglist_length(MsgList* thiz)
{
	size_t length = 0;
	MsgListNode* iter = thiz->first;

	while(iter != NULL)
	{
		length++;
		iter = iter->next;
	}

	return length;
}

MsgListRet msglist_foreach(MsgList* thiz, MsgListDataVisitFunc visit, void* ctx)
{
	MsgListRet ret = MSGLIST_RET_OK;
	MsgListNode* iter = thiz->first;

	while(iter != NULL && ret != MSGLIST_RET_STOP)
	{
		ret = visit(ctx, iter);
		iter = iter->next;
	}

	return ret;
}

int      msglist_find(MsgList* thiz, MsgListDataCompareFunc cmp, void* ctx)
{
	int i = 0;
	MsgListNode* iter = thiz->first;

	while(iter != NULL)
	{
		if(cmp(ctx, iter) == 0)
		{
			break;
		}
		i++;
		iter = iter->next;
	}

	return i;
}

void msglist_destroy(MsgList* thiz)
{
	MsgListNode* iter = thiz->first;
	MsgListNode* next = NULL;

	while(iter != NULL)
	{
		next = iter->next;
		msglist_node_destroy(iter);
		iter = next;
	}

	thiz->first = NULL;
	free(thiz);

	return;
}



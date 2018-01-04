
#include "CameraDebug.h"
#if DBG_BUFFER_LIST
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "BufferListManager"
#include <cutils/log.h>

#include "BufferListManager.h"

namespace android {

BufferListManager::BufferListManager()
	: mItemCnt(0)
{
	F_LOG;
	list_init(&mList);
}

BufferListManager::~BufferListManager()
{
	F_LOG;
	
	//struct listnode *node;
	buffer_node * alloc_buffer;

	/*
	list_for_each(node, &mList) 
	{
		alloc_buffer = node_to_item(node, buffer_node, i_list);
		if (alloc_buffer != NULL)
		{
			if (alloc_buffer->data != NULL)
			{
				LOGD("~BufferListManager free: %p", alloc_buffer->data);
				free(alloc_buffer->data);
				alloc_buffer->data = NULL;
			}

			free(alloc_buffer);
			alloc_buffer = NULL;
		}
	}*/

	while((alloc_buffer = pop()) != NULL)
	{
		releaseBuffer(alloc_buffer);
	}
}

buffer_node * BufferListManager::allocBuffer(uint32_t id, uint32_t min_size)
{
	Mutex::Autolock locker(&mLock);
	
	F_LOG;
	
	buffer_node * alloc_buffer = NULL;
	alloc_buffer = (buffer_node *)malloc(sizeof(buffer_node));
	if (alloc_buffer == NULL)
	{
		ALOGE("<F:%s, L:%d> Failed to alloc alloc_buffer!", __FUNCTION__, __LINE__);
		goto ALLOC_BUFFER_ERROR;
	}

	memset(alloc_buffer, 0, sizeof(buffer_node));
	alloc_buffer->data = (void*)malloc(min_size);
	if (alloc_buffer->data == NULL)
	{
		ALOGE("<F:%s, L:%d> Failed to alloc alloc_buffer->data!", __FUNCTION__, __LINE__);
		goto ALLOC_BUFFER_ERROR;
	}
	alloc_buffer->size = min_size;
	
	LOGV("allocBuffer: %p", alloc_buffer->data);
	
	return alloc_buffer;
	
ALLOC_BUFFER_ERROR:
	if (alloc_buffer != NULL)
	{
		if (alloc_buffer->data != NULL)
		{
			free(alloc_buffer->data);
			alloc_buffer->data = NULL;
		}

		free(alloc_buffer);
		alloc_buffer = NULL;
	}
	return NULL;
}

void BufferListManager::releaseBuffer(buffer_node * node)
{
	Mutex::Autolock locker(&mLock);

	F_LOG;
	
	if (node != NULL)
	{
		if (node->data != NULL)
		{
			LOGV("releaseBuffer: %p", node->data);
			free(node->data);
			node->data = NULL;
		}

		free(node);
		//node = NULL;
	}
}

bool BufferListManager::isListEmpty()
{
	Mutex::Autolock locker(&mLock);
	
	return list_empty(&mList);
}

buffer_node * BufferListManager::pop()
{
	Mutex::Autolock locker(&mLock);
	
	F_LOG;

	if(mItemCnt <= 0)
	{
		return NULL;
	}
	
	buffer_node * node = node_to_item(list_head(&mList), buffer_node, i_list);
	
	list_remove(&node->i_list);

	mItemCnt--;
	
	return node;
}

void BufferListManager::push(buffer_node * node)
{
	Mutex::Autolock locker(&mLock);
	
	F_LOG;
	
	list_add_tail(&mList, &node->i_list);
	mItemCnt++;
}

}

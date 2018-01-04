
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OSAL_Mutex.h"
#include "OSAL_Queue.h"


OMX_ERRORTYPE OSAL_QueueCreate(OSAL_QUEUE *queueHandle, int maxQueueElem)
{
    int i = 0;
    OSAL_QElem *newqelem = NULL;
    OSAL_QElem *currentqelem = NULL;
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    ret = OSAL_MutexCreate(&queue->qMutex);
    if (ret != OMX_ErrorNone)
        return ret;

	queue->maxElem = maxQueueElem;

    queue->first = (OSAL_QElem *)malloc(sizeof(OSAL_QElem));
    if (queue->first == NULL)
        return OMX_ErrorInsufficientResources;

    memset(queue->first, 0, sizeof(OSAL_QElem));
    currentqelem = queue->last = queue->first;
    queue->numElem = 0;

    for (i = 0; i < (queue->maxElem - 2); i++) {
        newqelem = (OSAL_QElem *)malloc(sizeof(OSAL_QElem));
        if (newqelem == NULL) {
            while (queue->first != NULL) {
                currentqelem = queue->first->qNext;
                free((OMX_PTR)queue->first);
                queue->first = currentqelem;
            }
            return OMX_ErrorInsufficientResources;
        } else {
            memset(newqelem, 0, sizeof(OSAL_QElem));
            currentqelem->qNext = newqelem;
            currentqelem = newqelem;
        }
    }

    currentqelem->qNext = queue->first;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OSAL_QueueTerminate(OSAL_QUEUE *queueHandle)
{
    int i = 0;
    OSAL_QElem *currentqelem = NULL;
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    for ( i = 0; i < (queue->maxElem - 2); i++) {
        currentqelem = queue->first->qNext;
        free(queue->first);
        queue->first = currentqelem;
    }

    if(queue->first) {
        free(queue->first);
        queue->first = NULL;
    }

    ret = OSAL_MutexTerminate(queue->qMutex);

    return ret;
}

int OSAL_Queue(OSAL_QUEUE *queueHandle, void *data)
{
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    OSAL_MutexLock(queue->qMutex);

    if ((queue->last->data != NULL) || (queue->numElem >= queue->maxElem)) {
        OSAL_MutexUnlock(queue->qMutex);
        return -1;
    }
    queue->last->data = data;
    queue->last = queue->last->qNext;
    queue->numElem++;

    OSAL_MutexUnlock(queue->qMutex);
    return 0;
}

void *OSAL_Dequeue(OSAL_QUEUE *queueHandle)
{
    void *data = NULL;
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;
    if (queue == NULL)
        return NULL;

    OSAL_MutexLock(queue->qMutex);

    if ((queue->first->data == NULL) || (queue->numElem <= 0)) {
        OSAL_MutexUnlock(queue->qMutex);
        return NULL;
    }
    data = queue->first->data;
    queue->first->data = NULL;
    queue->first = queue->first->qNext;
    queue->numElem--;

    OSAL_MutexUnlock(queue->qMutex);
    return data;
}

int OSAL_GetElemNum(OSAL_QUEUE *queueHandle)
{
    int ElemNum = 0;
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    OSAL_MutexLock(queue->qMutex);
    ElemNum = queue->numElem;
    OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

int OSAL_SetElemNum(OSAL_QUEUE *queueHandle, int ElemNum)
{
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    OSAL_MutexLock(queue->qMutex);
    queue->numElem = ElemNum; 
    OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

int OSAL_QueueSetElem(OSAL_QUEUE *queueHandle, void *data)
{
	int i = 0;
    OSAL_QUEUE *queue = (OSAL_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    OSAL_MutexLock(queue->qMutex);
	
    if ((queue->last->data != NULL) || (queue->numElem >= queue->maxElem)) {
        OSAL_MutexUnlock(queue->qMutex);
        return -1;
    }

	for (i = 0; i < queue->numElem; i++)
	{
		if ((queue->first->data == data)
			|| (queue->last->data == data))
		{
			// if there is an same elem, do not in queue anyway
			OSAL_MutexUnlock(queue->qMutex);
        	return 0;
		}
	}
	
    queue->last->data = data;
    queue->last = queue->last->qNext;
    queue->numElem++;

    OSAL_MutexUnlock(queue->qMutex);
    return 0;
}


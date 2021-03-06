/*******************************************************************************
 *
 * File: kernel_list.c
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 17/12/2017
 *
 * Version: 1.5
 *
 * Kernel priority lists used to manage data.
 ******************************************************************************/

#include "../lib/stddef.h"  /* OS_RETURN_E */
#include "../lib/stdint.h"  /* Generic int types */
#include "../lib/string.h"  /* memset */
#include "../memory/heap.h" /* kmalloc, kfree */

#include "../debug.h"       /* kernel_serial_debug */

/* Header include */
#include "kernel_list.h"

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

 kernel_list_node_t* kernel_list_create_node(void* data, OS_RETURN_E *error)
 {
     kernel_list_node_t* new_node;

     /* Create new node */
     new_node = kmalloc(sizeof(kernel_list_node_t));

     if(new_node == NULL)
     {
         if(error != NULL)
         {
             *error = OS_ERR_MALLOC;
         }
         return NULL;
     }

     /* Init the structure */
     memset(new_node, 0, sizeof(kernel_list_node_t));
     new_node->data = data;

     if(error != NULL)
     {
         *error = OS_NO_ERR;
     }

     return new_node;
}

OS_RETURN_E kernel_list_delete_node(kernel_list_node_t** node)
{
    if(node == NULL || *node == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Check list chaining */
    if((*node)->enlisted != 0)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    kfree(*node);

    *node = NULL;

    return OS_NO_ERR;
}

kernel_list_t* kernel_list_create_list(OS_RETURN_E *error)
{
    kernel_list_t* new_list;

    /* Create new node */
    new_list = kmalloc(sizeof(kernel_list_t));
    if(new_list == NULL)
    {
        if(error != NULL)
        {
            *error = OS_ERR_MALLOC;
        }
        return NULL;
    }

    /* Init the structure */
    memset(new_list, 0, sizeof(kernel_list_t));

    if(error != NULL)
    {
        *error = OS_NO_ERR;
    }

    return new_list;
}

OS_RETURN_E kernel_list_delete_list(kernel_list_t** list)
{
    if(list == NULL || *list == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Check list chaining */
    if((*list)->head != NULL || (*list)->tail != NULL)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    kfree(*list);

    *list = NULL;

    return OS_NO_ERR;
}
#include "kernel_thread.h"
OS_RETURN_E kernel_list_enlist_data(kernel_list_node_t* node,
                                    kernel_list_t* list,
                                    const uint16_t priority)
{
    kernel_list_node_t* cursor;

    #ifdef DEBUG_KERNEL_QUEUE
    kernel_serial_debug("Enlist 0x%08x in lsit 0x%08x\n",
                        (uint32_t)node,
                        (uint32_t)list);
    #endif

    if(node == NULL || list == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    node->priority = priority;

    /* If this queue is empty */
    if(list->head == NULL)
    {
        /* Set the first item */
        list->head = node;
        list->tail = node;
        node->next = NULL;
        node->prev = NULL;
    }
    else
    {
        cursor = list->head;
        while(cursor != NULL && cursor->priority > priority)
        {
            cursor = cursor->next;
        }

        if(cursor != NULL)
        {
            node->next = cursor;
            node->prev = cursor->prev;
            cursor->prev = node;
            if(node->prev != NULL)
            {
                node->prev->next = node;
            }
            else
            {
                list->head = node;
            }
        }
        else
        {
            /* Just put on the tail */
            node->prev = list->tail;
            node->next = NULL;
            list->tail->next = node;
            list->tail = node;
        }
    }
    ++list->size;
    node->enlisted = 1;

    if(node->next && node->prev && node->next == node->prev)
    {
        kernel_panic();
    }

    return OS_NO_ERR;
}

kernel_list_node_t* kernel_list_delist_data(kernel_list_t* list,
                                            OS_RETURN_E* error)
{
    kernel_list_node_t*  node;

    #ifdef DEBUG_KERNEL_QUEUE
    kernel_serial_debug("Delist kernel element in list 0x%08x\n",
                        (uint32_t)list);
    #endif

    if(list == NULL)
    {
        if(error != NULL)
        {
            *error = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    if(error != NULL)
    {
        *error = OS_NO_ERR;
    }


    /* If this priority queue is empty */
    if(list->head == NULL)
    {
        return NULL;
    }

    /* Dequeue the last item */
    node = list->tail;
    if(node->prev != NULL)
    {
        node->prev->next = NULL;
        list->tail = node->prev;
    }
    else
    {
        list->head = NULL;
        list->tail = NULL;
    }

    --list->size;

    node->next = NULL;
    node->prev = NULL;
    node->enlisted = 0;

    if(error != NULL)
    {
        *error = OS_NO_ERR;
    }

    return node;
}

kernel_list_node_t* kernel_list_find_node(kernel_list_t* list, void* data,
                                          OS_RETURN_E *error)
{
    kernel_list_node_t* node;

    #ifdef DEBUG_KERNEL_QUEUE
    kernel_serial_debug("Find kernel data 0x%08x in list 0x%08x\n",
                        (uint32_t)data,
                        (uint32_t)list);
    #endif

    /* If this priority queue is empty */
    if(list == NULL)
    {
        if(error != NULL)
        {
            *error = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    /* Search for the data */
    node = list->head;
    while(node != NULL && node->data != data)
    {
        node = node->next;
    }

    /* No such data */
    if(node == NULL)
    {
        if(error != NULL)
        {
            *error = OS_ERR_NO_SUCH_ID;
        }
        return NULL;
    }

    if(error != NULL)
    {
        *error = OS_NO_ERR;
    }

    return node;
}

OS_RETURN_E kernel_list_remove_node_from(kernel_list_t* list,
                                         kernel_list_node_t* node)
{
    kernel_list_node_t* cursor;
    if(list == NULL || node == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    #ifdef DEBUG_KERNEL_QUEUE
    kernel_serial_debug("Remove node kernel node 0x%08x in list 0x%08x\n",
                        (uint32_t)node,
                        (uint32_t)list);
    #endif

    /* Search for node in the list */
    cursor = list->head;
    while(cursor != NULL && cursor != node)
    {
        cursor = cursor->next;
    }

    if(cursor == NULL)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Manage link */
    if(cursor->prev != NULL && cursor->next != NULL)
    {
        cursor->prev->next = cursor->next;
        cursor->next->prev = cursor->prev;
    }
    else if(cursor->prev == NULL && cursor->next != NULL)
    {
        list->head = cursor->next;
        cursor->next->prev = NULL;
    }
    else if(cursor->prev != NULL && cursor->next == NULL)
    {
        list->tail = cursor->prev;
        cursor->prev->next = NULL;
    }
    else
    {
        list->head = NULL;
        list->tail = NULL;
    }

    node->next = NULL;
    node->prev = NULL;

    node->enlisted = 0;

    return OS_NO_ERR;
}

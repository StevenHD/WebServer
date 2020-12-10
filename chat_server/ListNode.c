//
// Created by hlhd on 2020/12/9.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ListNode.h"

Node *createlist_head()
{
    Node* head = (Node*)malloc(sizeof(Node));
    head->next = NULL;
    return head;
}

int getlist(Node* head, char* buf, int len)
{
    char tmp[8];
    int index = 0;

    Node* cur;
    cur = head->next;

    while (cur && index < len)
    {
        sprintf(tmp, "%04d", cur->socket);
        memcpy(buf + index, tmp, 4);
        index += 4;
        cur = cur->next;
    }

    return index;
}

int lenlist(Node* head)
{
    int len = 0;
    while (head = head->next) len ++;
    return len;
}

Node* insertlist(Node *head, int socket)
{
    Node* cur = malloc(sizeof(Node));
    cur->socket = socket;
    cur->next = head->next;
    head->next = cur;
    return cur;
}

Node *findlist(Node *head, int find)
{
    Node* cur = head;
    while (cur = cur->next)
    {
        if (cur->socket = find) return head;
    }

    return NULL;
}

int deletelist(Node* head, int del)
{
    Node* cur = head;
    Node* tmp = cur->next;
    while (tmp)
    {
        if (tmp->socket == del)
        {
            cur->next = tmp->next;
            free(tmp);
            return 1;
        }

        cur = tmp;
        tmp = tmp->next;
    }

    return 0;
}
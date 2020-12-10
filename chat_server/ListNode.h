//
// Created by hlhd on 2020/12/9.
//

#ifndef WEBSERVER_LISTNODE_H
#define WEBSERVER_LISTNODE_H

typedef struct node
{
    int socket;
    struct node* next;
}Node;

Node *createlist_head();

int getlist(Node* head, char *buf, int len);

int lenlist(Node* head);

Node* insertlist(Node *head, int socket);

Node* findlist(Node* head, int find);

int deletelist(Node* head, int del);

#endif //WEBSERVER_LISTNODE_H

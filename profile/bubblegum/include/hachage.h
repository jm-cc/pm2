/**********************************************************************
 * File  : hachage.h
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 26/04/2006
*********************************************************************/

#ifndef HACHAGE_H
#define HACHAGE_H

#include <malloc.h>
#include <time.h>

struct PairNode_tag;
typedef struct PairNode_tag PairNode;

struct PairNode_tag
{
      int data;
      int key;
      PairNode* next;
};

typedef struct HashTable_tag
{
      int size;
      int xor;
      PairNode** table;
} HashTable;


// retourne un indice haché à partir d'une donnée
int HashFunction(HashTable* ht, int key);

HashTable* NewHashTable(int size);
void DeleteHashTable(HashTable* ht);

int GetData(HashTable* ht, int key);
void AddPair(HashTable* ht, int key, int data);

#endif

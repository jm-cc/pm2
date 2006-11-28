/**********************************************************************
 * File  : hachage.h
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 26/04/2006
*********************************************************************/

#ifndef HACHAGE_H
#define HACHAGE_H

#include <stdlib.h>
#include <time.h>

struct PairNode_tag;
typedef struct PairNode_tag PairNode;

struct PairNode_tag
{
      long data;
      long key;
      PairNode* next;
};

typedef struct HashTable_tag
{
      long size;
      long xor;
      PairNode** table;
} HashTable;


// retourne un indice haché à partir d'une donnée
long HashFunction(HashTable* ht, long key);

HashTable* NewHashTable(long size);
void DeleteHashTable(HashTable* ht);

long GetData(HashTable* ht, long key);
void AddPair(HashTable* ht, long key, long data);

#endif

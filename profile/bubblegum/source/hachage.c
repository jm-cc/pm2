/**********************************************************************
 * File  : hachage.c
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 26/04/2006
 *********************************************************************/

#include "hachage.h"

/*********************************************************************/

long HashFunction(HashTable* ht, long key)
{
   key = key < 0 ? -key : key;
   return (key ^ ht->xor) % ht->size;
}

/*********************************************************************/

HashTable* NewHashTable(long size)
{
   HashTable* ht = malloc(sizeof(HashTable));

   ht->size = size;
   ht->table = calloc(sizeof(PairNode*), size);  // zero filled

   ht->xor = time(NULL);
   
   return ht;
}

/*********************************************************************/

void DeleteHashTable(HashTable* ht)
{
   long i;
   PairNode* pn, *pno;
   
   for (i = 0; i < ht->size; ++i)
   {
      if (ht->table[i] != NULL)   // si il y a une liste a supprimer sur cette case de la table
      {
         pn = ht->table[i];
         while (pn->next != NULL)
         {
            pno = pn;
            pn = pn->next;
            free(pno);  // on fait écrouler le pont derrière nos pas
         }
      }
   }
   free(ht);
}

/*********************************************************************/

long GetData(HashTable* ht, long key)
{
   long ind = HashFunction(ht, key);
   PairNode* pn;

   if (ht->table[ind] == NULL)
   {
      return -1;
   }
   else
   {
      pn = ht->table[ind];
      while (pn->key != key)
      {
         pn = pn->next;
         if (pn == NULL)  // pas normal
         {
            return -1;
         }            
      }
   }
   return pn->data;
}

/*********************************************************************/

void AddPair(HashTable* ht, long key, long data)
{
   long ind = HashFunction(ht, key);
   PairNode* pn;
   
   if (ht->table[ind] == NULL)
   {  // premier element sur cette case
      ht->table[ind] = malloc(sizeof(PairNode));
      ht->table[ind]->data = data;
      ht->table[ind]->key = key;
      ht->table[ind]->next = NULL;
   }
   else
   {  // rajouter dans la liste
      pn = ht->table[ind];
      while (pn->next != NULL)
      {
         if (pn->data == data)  // la donnée existe déjà
            return;
         pn = pn->next;
      }
      pn->next = malloc(sizeof(PairNode));
      pn = pn->next;
      pn->data = data;
      pn->key = key;
      pn->next = NULL;
   }   
}

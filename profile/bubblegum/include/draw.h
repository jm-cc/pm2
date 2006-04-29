
typedef struct _position
{
  int x;
  int y;
  int largeur;
  int hauteur;
} position;

typedef struct _liste_arbre_pos
{
  arbre_pos * tete;
  struct _liste_arbre_pos * reste;
} liste_arbre_pos;

typedef struct _arbre_pos
{
  position * pos;
  liste_arbre_pos* fils;
}arbre_pos;

typedef struct _tab
{
  int taille;
  int nb;
  int * elements;
} parcours;

position * CreatePosition()
int GetPosX(position *prtpos);
int GetPosY(position *prtpos);
int GetLargeur(position *prtpos);
int GetHauteur(position *prtpos);
void SetPosX(position *prtpos, int x);
void SetPosY(position *prtpos, int y);
void SetLargeur(position *prtpos, int l);
void SetHauteur(position *prtpos, int h);
void RemovePosition(position * pos);
int IsIn(position * pos, int x, int y);

liste_arbre_pos * CreateListeArbrePos();
void AddListeArbre(liste_arbre_pos * liste, arbre_pos * arbre);
arbre_pos * GetHead(liste_arbre_pos liste);
liste_arbre_pos * GetTail(liste_arbre_pos liste);
int IsInListe(liste_arbre_pos liste, int x, int y);
void RemoveArbreListe(liste_arbre_pos * liste, int pos);
void RemoveListe(liste_arbre_pos);

arbre_pos * CreateArbrePos();
position * GetPosition(arbre_pos * arbre);
void SetPosition(arbre_pos * arbre, position * pos);
void AddArbreArbre(arbre_pos * conteneur, arbre_pos * contenu);
void AddArbrePosition(arbre_pos * arbre, position * pos);
void RemoveArbre(arbre_pos * arbre);

parcours * CreateParcours();
void AddParcoursEntier(parcours * p, int i);
int GetEntier(parcours * p, int pos);
void RemoveParcours();

parcours * GetParcours(arbre_pos * arbre, int x, int y);
element * GetElementParcours(element * bulle, parcours * p);

position * CreatePositionThread(arbre_pos arbreprinc, int x, int y);
position * CreatePositionBulle(arbre_pos arbreprinc, int x, int y);
void Move(element * bulleprinc, arbre_pos * arbreprinc, position * src, position * dest);
void Resize(arbre_pos * arbreprinc, position * p);
void Delete(element * bulleprinc, arbre_pos * arbreprinc, position * p);

void DrawBulle(position * pos);
void DrawThread(position * pos);
void DrawArea(element * bulleprinc, arbre_pos * arbre);

#include "emg.h"

static int
_comic_chapter_item_sort_cb(Comic_Chapter_Item *a, Comic_Chapter_Item *b)
{
   return a->cc->number - b->cc->number;
}

static int
_comic_chapter_sort_cb(Comic_Chapter *a, Comic_Chapter *b)
{
   return a->provider->priority - b->provider->priority;
}

static void
_comic_chapter_item_update(Comic_Chapter_Item *cci, Comic_Chapter *cc, Eina_Bool force)
{
#define SET(VAR) \
    if (cc->VAR || force) \
      cci->VAR = cc->VAR

    cc->cci = cci;
    if (cci->cc && (cci->cc->provider->priority > cc->provider->priority))
      {
         SET(name);
         SET(date);
         if (cci->it)
           elm_genlist_item_update(cci->it);
         return;
      }
    //DBG("new cc for %g: %s", cc->number, cc->href);
    cci->cc = cc;
    SET(name);
    SET(date);
    if (cci->it)
      elm_genlist_item_update(cci->it);
}

static Comic_Chapter_Item *
_comic_chapter_item_new(Comic_Chapter *cc)
{
   Comic_Chapter_Item *cci;

   cci = calloc(1, sizeof(Comic_Chapter_Item));
   //DBG("new item: cc %g", cc->number);
   _comic_chapter_item_update(cci, cc, EINA_TRUE);
   cc->csd->chapters = eina_inlist_remove(cc->csd->chapters, EINA_INLIST_GET(cc));
   cc->csd->cs->chapters = eina_inlist_sorted_insert(cc->csd->cs->chapters, EINA_INLIST_GET(cci), (Eina_Compare_Cb)_comic_chapter_item_sort_cb);
   cci->chapters = eina_inlist_append(cci->chapters, EINA_INLIST_GET(cc));
   cci->chapter_count++;
   return cci;
}

Comic_Chapter_Item *
comic_chapter_item_chapter_add(Comic_Chapter *cc, Comic_Chapter_Item *cci)
{
   Eina_Bool forward = EINA_TRUE;

   DBG("(cc=%g, cci=%g)", cc->number, cci ? cci->cc->number : 0);
   if (!cc->csd->cs->chapters)
     return _comic_chapter_item_new(cc);

   if (!cci)
     {
        if (cc->number > cc->csd->cs->total / 2)
          {
             cci = EINA_INLIST_CONTAINER_GET(cc->csd->cs->chapters->last, Comic_Chapter_Item);
             forward = EINA_FALSE;
          }
        else
          cci = EINA_INLIST_CONTAINER_GET(cc->csd->cs->chapters, Comic_Chapter_Item);
     }
   if (cci->cc->number > cc->number)
     forward = EINA_FALSE;
   if (forward)
     {
        DBG("ITER: FORWARD");
        for (; cci; cci = comic_chapter_item_next_get(cci))
          {
             if (cci->cc->number < cc->number) continue;
             cc->csd->chapters = eina_inlist_remove(cc->csd->chapters, EINA_INLIST_GET(cc));
             //DBG("INSERT: %g", cci->cc->number);
             cci->chapters = eina_inlist_sorted_insert(cci->chapters, EINA_INLIST_GET(cc), (Eina_Compare_Cb)_comic_chapter_sort_cb);
             cci->chapter_count++;
             _comic_chapter_item_update(cci, cc, EINA_FALSE);
             return cci;
          }
        return _comic_chapter_item_new(cc);
     }
    //DBG("ITER: BACKWARD");
    for (; cci; cci = comic_chapter_item_prev_get(cci))
      {
         if (cci->cc->number > cc->number) continue;
         cc->csd->chapters = eina_inlist_remove(cc->csd->chapters, EINA_INLIST_GET(cc));
         //DBG("INSERT: %g", cci->cc->number);
         cci->chapters = eina_inlist_sorted_insert(cci->chapters, EINA_INLIST_GET(cc), (Eina_Compare_Cb)_comic_chapter_sort_cb);
         cci->chapter_count++;
         _comic_chapter_item_update(cci, cc, EINA_FALSE);
         return cci;
      }
   return _comic_chapter_item_new(cc);
}

void
comic_chapter_item_chapter_del(Comic_Chapter *cc)
{
   Comic_Chapter_Item *cci = cc->cci;

   cci->chapters = eina_inlist_remove(cci->chapters, EINA_INLIST_GET(cc));
   cci->chapter_count--;
   if (cc != cci->cc) return;

   if (cci->chapters)
     {
        cci->cc = EINA_INLIST_CONTAINER_GET(cci->chapters, Comic_Chapter);
        _comic_chapter_item_update(cci, cci->cc, EINA_TRUE);
        return;
     }

   cc->csd->cs->chapters = eina_inlist_remove(cc->csd->cs->chapters, EINA_INLIST_GET(cci));
   free(cci);
}

void
comic_chapter_item_update(Comic_Chapter *cc)
{
   if (!cc->cci) return;
   _comic_chapter_item_update(cc->cci, cc, EINA_FALSE);
}

Comic_Chapter_Item *
comic_chapter_item_prev_get(Comic_Chapter_Item *cci)
{
   if (!EINA_INLIST_GET(cci)->prev) return NULL;
   return EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cci)->prev, Comic_Chapter_Item);
}

Comic_Chapter_Item *
comic_chapter_item_next_get(Comic_Chapter_Item *cci)
{
   if (!EINA_INLIST_GET(cci)->next) return NULL;
   return EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cci)->next, Comic_Chapter_Item);
}

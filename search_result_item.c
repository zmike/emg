#include "emg.h"

static int
_search_result_item_insert(Search_Result *a, Search_Result *b)
{
   return a->provider->priority - b->provider->priority;
}

static void
_search_result_item_update(Search_Result_Item *sri, Search_Result *sr, Eina_Bool force)
{
#define SET(VAR) \
    if (sr->VAR || force) \
      sri->VAR = sr->VAR

    sr->sri = sri;
    if (sri->sr && (sri->sr->provider->priority > sr->provider->priority))
      {
         if (!sri->tags)
           {
              SET(tags);
              SET(tags_len);
           }
         if (!sri->total)
           {
              SET(total);
           }
         if ((!sri->image) && (sr->image.buf || sr->image.ecu))
           sri->image = &sr->image;
         elm_genlist_item_update(sri->it);
         return;
      }
    sri->sr = sr;
    if (sr->name || force)
      {
         SET(name);
         SET(namelen);
      }
    SET(href);
    SET(total);
    if (force || (sr->image.buf || sr->image.ecu))
      sri->image = &sr->image;
    if (sr->tags || force)
      {
         SET(tags);
         SET(tags_len);
      }
    if (sri->it)
      elm_genlist_item_update(sri->it);
    else
      sri->it = elm_genlist_item_append(sri->sr->e->sw.list, &sri->sr->e->sw.itc, sri, NULL, 0, NULL, NULL);
}

static void
_search_result_item_new(Search_Result *sr)
{
   Search_Result_Item *sri;

   sri = calloc(1, sizeof(Search_Result_Item));
   _search_result_item_update(sri, sr, EINA_FALSE);
   sr->e->sw.results = eina_list_append(sr->e->sw.results, sri);
   sri->results = eina_list_append(sri->results, sr);
}

void
search_result_item_result_add(Search_Result *sr)
{
   Eina_List *l;
   Search_Result_Item *sri;

   EINA_LIST_FOREACH(sr->e->sw.results, l, sri)
     {
        if (strcasecmp(sr->name, sri->name)) continue;
        _search_result_item_update(sri, sr, EINA_FALSE);

        if (sri->sr == sr)
          sri->results = eina_list_prepend(sri->results, sr);
        else
          sri->results = eina_list_sorted_insert(sri->results, (Eina_Compare_Cb)_search_result_item_insert, sr);
        sri->result_count++;
        return;
     }
   _search_result_item_new(sr);
}

void
search_result_item_result_del(Search_Result *sr)
{
   Search_Result_Item *sri = sr->sri;

   sri->results = eina_list_remove(sri->results, sr);
   sri->result_count--;
   if (sr != sri->sr) return;

   sri->sr = eina_list_data_get(sri->results);
   if (sri->sr)
     {
        _search_result_item_update(sri, sri->sr, EINA_TRUE);
        return;
     }

   sr->e->sw.results = eina_list_remove(sr->e->sw.results, sri);
   free(sri);
}

void
search_result_item_update(Search_Result *sr)
{
   if (!sr->sri) return;
   _search_result_item_update(sr->sri, sr, EINA_FALSE);
}

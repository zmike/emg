#include "emg.h"

static int
_update_result_item_insert(Update_Result *a, Update_Result *b)
{
   return a->provider->priority - b->provider->priority;
}

static void
_update_result_item_update(Update_Result_Item *uri, Update_Result *ur, Eina_Bool force)
{
#define SET(VAR) \
    if (ur->VAR || force) \
      uri->VAR = ur->VAR

    ur->uri = uri;
    if (uri->ur && (uri->ur->provider->priority > ur->provider->priority))
      {
         elm_genlist_item_update(uri->it);
         return;
      }
    uri->ur = ur;
    if (ur->name || force)
      {
         SET(name);
         SET(namelen);
      }
    SET(href);
    if (uri->it)
      elm_genlist_item_update(uri->it);
    else
      uri->it = elm_genlist_item_append(uri->ur->e->uv.list[0], &uri->ur->e->uv.itc, uri, NULL, 0, NULL, NULL);
}

static void
_update_result_item_new(Update_Result *ur)
{
   Update_Result_Item *uri;

   uri = calloc(1, sizeof(Update_Result_Item));
   _update_result_item_update(uri, ur, EINA_TRUE);
   ur->e->uv.results = eina_list_append(ur->e->uv.results, uri);
   uri->results = eina_list_append(uri->results, ur);
}

void
update_result_item_result_add(Update_Result *ur)
{
   Eina_List *l;
   Update_Result_Item *uri;

   EINA_LIST_FOREACH(ur->e->uv.results, l, uri)
     {
        if (strcasecmp(ur->name, uri->name)) continue;
        _update_result_item_update(uri, ur, EINA_FALSE);

        if (uri->ur == ur)
          uri->results = eina_list_prepend(uri->results, ur);
        else
          uri->results = eina_list_sorted_insert(uri->results, (Eina_Compare_Cb)_update_result_item_insert, ur);
        return;
     }
   _update_result_item_new(ur);
}

void
update_result_item_result_del(Update_Result *ur)
{
   Update_Result_Item *uri = ur->uri;

   uri->results = eina_list_remove(uri->results, ur);
   if (ur != uri->ur) return;

   uri->ur = eina_list_data_get(uri->results);
   if (uri->ur)
     {
        _update_result_item_update(uri, uri->ur, EINA_TRUE);
        return;
     }

   ur->e->uv.results = eina_list_remove(ur->e->uv.results, uri);
   free(uri);
}

void
update_result_item_update(Update_Result *ur)
{
   if (!ur->uri) return;
   _update_result_item_update(ur->uri, ur, EINA_FALSE);
}

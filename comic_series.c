#include "emg.h"

Comic_Series *
comic_series_find(EMG *e, const char *name)
{
   Eina_List *l;
   Comic_Series *cs;


   if ((!name) || (!name[0])) return NULL;
   EINA_LIST_FOREACH(e->series, l, cs)
     {
        if (!strcasecmp(cs->name, name)) return cs;
     }
   return NULL;
}

Comic_Series *
comic_series_create(Search_Result *sr)
{
   Comic_Series *cs;
   const char *buf;

   cs = calloc(1, sizeof(Comic_Series));

   cs->identifier = IDENTIFIER_COMIC_SERIES;
   cs->e = sr->e;
   if (*sr->search == IDENTIFIER_SEARCH_NAME)
     {
        Search_Name *sn;

        sn = (Search_Name*)sr->search;
        sn->provider.init_cb(cs);
     }
   /* FIXME: use aggregator to ensure all chapters */
   cs->total = sr->total;
   cs->name = eina_stringshare_ref(sr->name);
   cs->provider.url = sr->provider_url;
   cs->namelen = sr->namelen;
   cs->image.identifier = IDENTIFIER_COMIC_IMAGE;
   cs->image.parent = cs;

   buf = eina_stringshare_printf("%s%s", sr->provider_url, sr->href);
   cs->ecu = ecore_con_url_new(buf);
   eina_stringshare_del(buf);
   ecore_con_url_data_set(cs->ecu, cs);
   ecore_con_url_get(cs->ecu);
   return cs;
}

void
comic_series_data(Comic_Series *cs, Ecore_Con_Event_Url_Data *ev)
{
   cs->provider.data_cb(cs, ev);
}

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
comic_series_new(Search_Result_Item *sri)
{
   Comic_Series *cs;
   Eina_List *l;
   Search_Result *sr;

   cs = calloc(1, sizeof(Comic_Series));

   cs->e = sri->sr->e;
   /* FIXME: use aggregator to ensure all chapters */
   cs->total = sri->total;
   cs->name = eina_stringshare_ref(sri->name);
   cs->namelen = sri->namelen;

   EINA_LIST_FOREACH(sri->results, l, sr)
     {
        Comic_Series_Data *csd;

        csd = calloc(1, sizeof(Comic_Series_Data));
        csd->identifier = IDENTIFIER_COMIC_SERIES_DATA;
        csd->cs = cs;
        csd->provider = sr->provider->init_cb();
        csd->image.identifier = IDENTIFIER_COMIC_SERIES_IMAGE;
        csd->image.parent = csd;
        csd->ecu = ecore_con_url_new(sr->href);
        ecore_con_url_data_set(csd->ecu, csd);
        if (!ecore_con_url_get(csd->ecu)) abort();
        cs->providers = eina_list_append(cs->providers, csd);
        if ((!cs->csd) || (cs->csd->provider->priority < csd->provider->priority))
          cs->csd = csd;
     }
   return cs;
}

void
comic_series_parser(Comic_Series_Data *csd)
{
   csd->provider->data_cb(csd);
}

Comic_Chapter *
comic_series_chapter_first_get(Comic_Series *cs)
{
   if (!cs->chapters) return NULL;
   return EINA_INLIST_CONTAINER_GET(cs->chapters, Comic_Chapter);
}

Comic_Chapter *
comic_series_chapter_last_get(Comic_Series *cs)
{
   if (!cs->chapters) return NULL;
   return EINA_INLIST_CONTAINER_GET(cs->chapters->last, Comic_Chapter);
}

Eina_Bool
comic_series_data_current(Comic_Series_Data *csd)
{
   return csd->cs->csd == csd;
}

void
comic_series_year_set(Comic_Series_Data *csd, const char *index_start)
{
   Eina_Bool current = comic_series_data_current(csd);

   if (current || (!csd->cs->year))
     {
        errno = 0;
        csd->cs->year = strtoul((char*)index_start, NULL, 10);
        if (errno) {/* FIXME */}
     }
   INF("year=%u", csd->cs->year);
}

void
comic_series_desc_set(Comic_Series_Data *csd, const char *index_start, const char *p)
{
   Eina_Bool current = comic_series_data_current(csd);

   if (current)
     {
        eina_stringshare_del(csd->cs->desc);
        csd->cs->desc = util_markup_to_utf8(index_start, p);
     }
   else if (!csd->cs->desc)
     csd->cs->desc = util_markup_to_utf8(index_start, p);
   //INF("desc=%s", csd->cs->desc);
}

void
comic_series_artist_set(Comic_Series_Data *csd, const char *index_start, const char *p)
{
   Eina_Bool current = comic_series_data_current(csd);

   if (current)
     {
        eina_stringshare_del(csd->cs->artist);
        csd->cs->artist = eina_stringshare_add_length(index_start, p - index_start);
     }
   else if (!csd->cs->artist)
     csd->cs->artist = eina_stringshare_add_length(index_start, p - index_start);
   INF("artist=%s", csd->cs->artist);
}

void
comic_series_author_set(Comic_Series_Data *csd, const char *index_start, const char *p)
{
   Eina_Bool current = comic_series_data_current(csd);

   if (current)
     {
        eina_stringshare_del(csd->cs->author);
        csd->cs->author = eina_stringshare_add_length(index_start, p - index_start);
     }
   else if (!csd->cs->author)
     csd->cs->author = eina_stringshare_add_length(index_start, p - index_start);
   INF("author=%s", csd->cs->author);
}

void
comic_series_alt_name_set(Comic_Series_Data *csd, const char *index_start, const char *p)
{
   Eina_Bool current = comic_series_data_current(csd);

   if (current)
     {
        eina_stringshare_del(csd->cs->alt_name);
        csd->cs->alt_name = eina_stringshare_add_length(index_start, p - index_start);
     }
   else if (!csd->cs->alt_name)
     csd->cs->alt_name = eina_stringshare_add_length(index_start, p - index_start);
   INF("alt_name=%s", csd->cs->alt_name);
}

void
comic_series_image_fetch(Comic_Series_Data *csd)
{
   csd->image.ecu = ecore_con_url_new(csd->image.href);
   ecore_con_url_data_set(csd->image.ecu, &csd->image);
   if (!ecore_con_url_get(csd->image.ecu)) abort();
}

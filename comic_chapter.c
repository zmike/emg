#include "emg.h"

Comic_Chapter *
comic_chapter_new(Comic_Series *cs, Eina_Bool before)
{
   Comic_Chapter *cc;

   cc = calloc(1, sizeof(Comic_Chapter));
   cc->cs = cs;
   cc->identifier = IDENTIFIER_COMIC_CHAPTER;
   if (before)
     cs->chapters = eina_inlist_prepend(cs->chapters, EINA_INLIST_GET(cc));
   else
     cs->chapters = eina_inlist_append(cs->chapters, EINA_INLIST_GET(cc));
   return cc;
}

void
comic_chapter_clear(Comic_Chapter *cc)
{
   Comic_Page *cp;

   EINA_INLIST_FOREACH(cc->pages, cp)
     {
        if (cp->nf_it) elm_object_item_del(cp->nf_it);
        if (cp->image.buf) eina_binbuf_free(cp->image.buf);
        cp->obj = NULL;
        cp->image.buf = NULL;
        cp->nf_it = NULL;
     }
   cc->pages_fetched = 0;
}


Comic_Chapter *
comic_chapter_prev_get(Comic_Chapter *cc)
{
   if (!EINA_INLIST_GET(cc)->prev) return NULL;
   return EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cc)->prev, Comic_Chapter);
}

Comic_Chapter *
comic_chapter_next_get(Comic_Chapter *cc)
{
   if (!EINA_INLIST_GET(cc)->next) return NULL;
   return EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cc)->next, Comic_Chapter);
}

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
comic_chapter_data_clear(Comic_Chapter *cc)
{
   Comic_Page *cp;

   if (!cc->pages) return;
   EINA_INLIST_FOREACH(cc->pages, cp)
     comic_page_data_del(cp);
}

void
comic_chapter_images_clear(Comic_Chapter *cc)
{
   Comic_Page *cp;

   if (!cc->pages) return;
   EINA_INLIST_FOREACH(cc->pages, cp)
     {
        comic_page_image_del(cp);
        DBG("DEL IMG: %u", cp->number);
     }
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

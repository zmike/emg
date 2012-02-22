#include "emg.h"

Comic_Chapter *
comic_chapter_new(Comic_Series *cs)
{
   Comic_Chapter *cc;

   cc = calloc(1, sizeof(Comic_Chapter));
   cc->cs = cs;
   cc->identifier = IDENTIFIER_COMIC_CHAPTER;
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

#include "emg.h"

Comic_Page *
comic_page_new(Comic_Chapter *cc, unsigned int id)
{
   Comic_Page *cp;
   Eina_Bool in = EINA_FALSE;

   cp = calloc(1, sizeof(Comic_Page));
   cp->cc = cc;
   cp->identifier = IDENTIFIER_COMIC_PAGE;
   cp->image.identifier = IDENTIFIER_COMIC_PAGE_IMAGE;
   cp->image.parent = cp;
   cp->number = id;
   INF("NEW PAGE FOR %g: %u", cc->number, id);
   cc->page_count++;
   if (cc->pages)
     {
        Comic_Page *cf;
        cf = EINA_INLIST_CONTAINER_GET(cc->pages, Comic_Page);
        if (cf->number > cp->number)
          {
             EINA_INLIST_REVERSE_FOREACH(cc->pages, cf)
               {
                  if (cf->number < cp->number)
                    {
                       cc->pages = eina_inlist_append_relative(cc->pages, EINA_INLIST_GET(cp), EINA_INLIST_GET(cf));
                       in = EINA_TRUE;
                    }
               }
             if (!in)
               cc->pages = eina_inlist_prepend(cc->pages, EINA_INLIST_GET(cp));
             in = EINA_TRUE;
          }
     }
   if (!in)
     cc->pages = eina_inlist_append(cc->pages, EINA_INLIST_GET(cp));
   cc->cs->provider->init_cb(cp);
   return cp;
}

void
comic_page_fetch(Comic_Page *cp)
{
   Ecore_Con_Url *ecu;

   if ((!cp->href) && (!cp->image.href)) return;
   if (cp->ecu || cp->image.ecu) return;

   if (cp->image.href)
     {
        cp->image.ecu = ecu = ecore_con_url_new(cp->image.href);
        ecore_con_url_data_set(cp->image.ecu, &cp->image);
     }
   else
     {
        cp->ecu = ecu = ecore_con_url_new(cp->href);
        ecore_con_url_data_set(cp->ecu, cp);
     }
   if (!ecore_con_url_get(ecu)) abort(); /* FIXME */
   if (cp->cc->decimal)
     INF("PAGE FETCH: %s - %g:%u: %s", cp->cc->cs->name, cp->cc->number, cp->number, ecore_con_url_url_get(ecu));
   else
     INF("PAGE FETCH: %s - %d:%u: %s", cp->cc->cs->name, (int)cp->cc->number, cp->number, ecore_con_url_url_get(ecu));
}

void
comic_page_data_del(Comic_Page *cp)
{
   if (!cp->image.buf) return;
   eina_binbuf_free(cp->image.buf);
   cp->image.buf = NULL;
   cp->cc->pages_fetched--;
}

void
comic_page_image_del(Comic_Page *cp)
{
    if (cp->nf_it) elm_object_item_del(cp->nf_it);
    cp->obj = cp->scr = NULL;
    cp->nf_it = NULL;
}

void
comic_page_parser(Comic_Page *cp)
{
   cp->provider->data_cb(cp);
}

Comic_Page *
comic_page_prev_get(Comic_Page *cp)
{
   if (!EINA_INLIST_GET(cp)->prev)
     {
        Comic_Chapter *cc;

        if (!EINA_INLIST_GET(cp->cc)->prev) return NULL;
        cc = EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cp->cc)->prev, Comic_Chapter);
        if (!cc->pages) return NULL;
        return EINA_INLIST_CONTAINER_GET(cc->pages->last, Comic_Page);
     }
   return EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cp)->prev, Comic_Page);
}

Comic_Page *
comic_page_next_get(Comic_Page *cp)
{
   if (!EINA_INLIST_GET(cp)->next)
     {
        Comic_Chapter *cc;

        if (!EINA_INLIST_GET(cp->cc)->next) return NULL;
        cc = EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cp->cc)->next, Comic_Chapter);
        if (!cc->pages) return NULL;
        return EINA_INLIST_CONTAINER_GET(cc->pages, Comic_Page);
     }
   return EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cp)->next, Comic_Page);
}

Eina_Bool
comic_page_current(Comic_Page *cp)
{
   return cp == cp->cc->cs->e->cv.cc->current;
}

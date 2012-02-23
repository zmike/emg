#include "emg.h"

void
comic_view_readahead_ensure(EMG *e)
{
   Comic_Page *cp;
   unsigned int x;

   cp = comic_page_next_get(e->cv.cc->current);
   if (!cp) return;
   for (x = 0; (x < DEFAULT_PAGE_READAHEAD) && cp; x++, cp = comic_page_next_get(cp))
     {
        if ((!cp->image.ecu) && (!cp->image.buf) && (!cp->buf))
          comic_page_fetch(cp);
     }
   cp = comic_page_prev_get(e->cv.cc->current);
   if (!cp) return;
   for (x = 0; (x < DEFAULT_PAGE_READBEHIND) && cp; x++, cp = comic_page_prev_get(cp))
     {
        if ((!cp->image.ecu) && (!cp->image.buf) && (!cp->buf))
          comic_page_fetch(cp);
     }
}

void
comic_view_show(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *event_info __UNUSED__)
{
   if (!event_info)
     {
        elm_toolbar_item_selected_set(e->cv.tb_it, EINA_TRUE);
        return;
     }
   if (e->view == EMG_VIEW_READER) return;
   elm_frame_collapse_go(e->sw.fr, EINA_TRUE);
   elm_naviframe_item_simple_promote(e->nf, e->cv.nf);
   {
      Evas *evas;
      Evas_Modifier_Mask ctrl, shift, alt;
      evas = evas_object_evas_get(e->win);
      ctrl = evas_key_modifier_mask_get(evas, "Control");
      shift = evas_key_modifier_mask_get(evas, "Shift");
      alt = evas_key_modifier_mask_get(evas, "Alt");
      evas_object_key_ungrab(e->win, "KP_Enter", 0, ctrl | shift | alt); /* worst warn_unused ever. */
      evas_object_key_ungrab(e->win, "Return", 0, ctrl | shift | alt); /* worst warn_unused ever. */
      1 | evas_object_key_grab(e->win, "KP_Space", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
      1 | evas_object_key_grab(e->win, "KP_Right", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
      1 | evas_object_key_grab(e->win, "KP_Left", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
      1 | evas_object_key_grab(e->win, "Right", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
      1 | evas_object_key_grab(e->win, "Left", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
   }
   e->view = EMG_VIEW_READER;
}

void
comic_view_page_set(EMG *e, Comic_Page *cp)
{
   char *buf;
   size_t size;
   Evas_Object *next, *prev;
   Comic_Page *cpn, *cpp;

   cp->cc->current = cp;
   e->cv.cc = cp->cc;
   if (cp->image.ecu) return; /* currently downloading */
   if ((!cp->obj) && (!cp->image.buf))
     {
        comic_page_fetch(cp);
        return;
     }

   /* fetch readahead pages on every page set */
   comic_view_readahead_ensure(e);
   if (cp->obj && cp->nf_it)
     {
        cpn = comic_page_next_get(cp);
        cpp = comic_page_prev_get(cp);
        INF("PREV PAGE: %u; CURRENT PAGE: %u; NEXT PAGE: %u", cpp ? (cpp->number) : 0, cp->number, cpn ? (cpn->number) : 0);
        elm_naviframe_item_promote(cp->nf_it);
        return;
     }
        
   cp->obj = elm_icon_add(e->win);
   WEIGHT(cp->obj, 0, 0);
   FILL(cp->obj);
   elm_icon_animated_set(cp->obj, EINA_TRUE);
   elm_icon_aspect_fixed_set(cp->obj, EINA_TRUE);
   elm_icon_fill_outside_set(cp->obj, EINA_FALSE);
   elm_icon_memfile_set(cp->obj, eina_binbuf_string_get(cp->image.buf), eina_binbuf_length_get(cp->image.buf), NULL, NULL);

   size = cp->cc->cs->namelen + sizeof(" ABCD:  - XYZ") + (cp->cc->name ? strlen(cp->cc->name) : 0);
   buf = alloca(size);
   if (cp->cc->decimal)
     snprintf(buf, size, "%s %g%s%s - %u", cp->cc->cs->name, cp->cc->number, cp->cc->name ? ": " : "", cp->cc->name ?: "", cp->number);
   else
     snprintf(buf, size, "%s %d%s%s - %u", cp->cc->cs->name, (int)cp->cc->number, cp->cc->name ? ": " : "", cp->cc->name ?: "", cp->number);
   cpn = comic_page_next_get(cp);
   cpp = comic_page_prev_get(cp);
   next = cpn ? e->cv.next : NULL;
   prev = cpp ? e->cv.prev : NULL;
   INF("PREV PAGE: %u; CURRENT PAGE: %u; NEXT PAGE: %u", prev ? (cpp->number) : 0, cp->number, next ? (cpn->number) : 0);
   cp->nf_it = elm_naviframe_item_push(e->cv.nf, buf, prev, next, cp->obj, NULL);
   elm_object_focus_set(e->cv.next, EINA_TRUE);
   evas_object_show(cp->obj);
   eina_binbuf_free(cp->image.buf);
   cp->image.buf = NULL;
}

void
comic_view_chapter_set(EMG *e, Comic_Chapter *cc)
{
   Comic_Page *cp;
   if (e->cv.cc && (e->cv.cc != cc))
     ecore_job_add((Ecore_Cb)comic_chapter_clear, e->cv.cc);
   cc->cs->current = e->cv.cc = cc;
   if (cc->pages)
     {
        cp = EINA_INLIST_CONTAINER_GET(cc->pages, Comic_Page);
        comic_view_page_set(e, cp);
        return;
     }
   cp = comic_page_new(cc, 1);
   cp->href = eina_stringshare_ref(cc->href);
   comic_view_page_set(e, cp);
   comic_page_fetch(cp);
}

void
comic_view_page_prev(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Comic_Page *cp;

   if ((!e->cv.cc) || (!e->cv.cc->current)) return;

   cp = comic_page_prev_get(e->cv.cc->current);
   if (!cp) return;
   comic_view_page_set(e, cp);
}

void
comic_view_page_next(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Comic_Page *cp;

   if ((!e->cv.cc) || (!e->cv.cc->current)) return;

   cp = comic_page_next_get(e->cv.cc->current);
   if (!cp) return;
   comic_view_page_set(e, cp);
}


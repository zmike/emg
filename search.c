#include "emg.h"

void
search_view_count_update(Search_Name *sn)
{
   unsigned int count;
   char buf[128];

   count = elm_genlist_items_count(sn->e->sw.list);
   if (sn->result_count == count) return;

   snprintf(buf, sizeof(buf), "Search Results (%u)", count);
   elm_object_item_text_set(sn->e->sw.tb_it, buf);
}

void
search_view_show(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *event_info __UNUSED__)
{
   if (!e->sw.nf_it) return;
   if (!event_info)
     {
        elm_toolbar_item_selected_set(e->sw.tb_it, EINA_TRUE);
        return;
     }
   elm_frame_collapse_go(e->sw.fr, EINA_FALSE);
   elm_naviframe_item_promote(e->sw.nf_it);
   if (e->view == EMG_VIEW_READER)
     {
        Evas *evas;
        Evas_Modifier_Mask ctrl, shift, alt;
        evas = evas_object_evas_get(e->win);
        ctrl = evas_key_modifier_mask_get(evas, "Control");
        shift = evas_key_modifier_mask_get(evas, "Shift");
        alt = evas_key_modifier_mask_get(evas, "Alt");
        1 | evas_object_key_grab(e->win, "Return", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
        1 | evas_object_key_grab(e->win, "KP_Enter", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
        evas_object_key_ungrab(e->win, "KP_Space", 0, ctrl | shift | alt);
        evas_object_key_ungrab(e->win, "KP_Right", 0, ctrl | shift | alt);
        evas_object_key_ungrab(e->win, "KP_Left", 0, ctrl | shift | alt);
        evas_object_key_ungrab(e->win, "Right", 0, ctrl | shift | alt);
        evas_object_key_ungrab(e->win, "Left", 0, ctrl | shift | alt);
     }
   e->view = EMG_VIEW_SEARCH;
}

void
search_view_clear(EMG *e)
{
   Search_Name *sn;

   if (!e->sw.searches) return;

   elm_genlist_clear(e->sw.list);
   EINA_LIST_FREE(e->sw.searches, sn)
     search_name_free(sn);
}

void
search_result_pick(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *it)
{
   Search_Result *sr = elm_object_item_data_get(it);
   Comic_Series *cs;

   cs = comic_series_find(sr->e, sr->name);
   series_view_clear(sr->e);
   if (cs)
     {
        series_view_populate(cs);
        return;
     }
   cs = comic_series_new(sr);
   series_view_title_set(e, cs);
   cs->e->sv.cs = cs;
   series_view_show(e, NULL, NULL);
}

void
search_name_create(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   const char *name;
   char *buf, *p, *pp;
   size_t len = 0;
   Ecore_Con_Url *url;
   Search_Name *sn;
   Eina_List *l;
   Provider_Init_Cb cb;

   name = elm_entry_entry_get(e->sw.entry);
   if ((!name) || (!name[0])) return;
   buf = evas_textblock_text_markup_to_utf8(NULL, name);
   if ((!buf) || (!buf[0])) return;

   search_view_clear(e);
   elm_toolbar_item_selected_set(e->sw.tb_it, EINA_TRUE);
   elm_object_disabled_set(e->sw.entry, EINA_TRUE);
   EINA_LIST_FOREACH(e->providers, l, cb)
     {
        sn = calloc(1, sizeof(Search_Name));
        e->sw.searches = eina_list_append(e->sw.searches, sn);
        sn->identifier = IDENTIFIER_SEARCH_NAME;
        sn->name = eina_stringshare_add(buf);
        sn->namelen = strlen(buf);
        e->sw.running++;
        cb(sn);
        sn->e = e;

        if (sn->provider.replace_str[1])
          {
             Eina_Strbuf *sbuf;

             sbuf = eina_strbuf_manage_new(buf);
             eina_strbuf_replace_all(sbuf, " ", sn->provider.replace_str);
             len = sn->snamelen = eina_strbuf_length_get(sbuf);
             buf = eina_strbuf_string_steal(sbuf);
             eina_strbuf_free(sbuf);
          }
        else
          {
             /* avoid realloc */
             for (p = pp = strchr(buf, ' '); pp; pp = strchr(++p, ' '))
               {
                  pp[0] = sn->provider.replace_str[0];
                  p = pp;
               }
             sn->snamelen = sn->namelen;
          }
        /* avoid strlen if possible */
        if (!len)
          {
             if (p)
               {
                  len = p - buf;
                  len += strlen(p);
               }
             else
               len = sn->namelen;
          }

        p = alloca(len += strlen(sn->provider.search_url) + 1);
        snprintf(p, len, sn->provider.search_url, buf);
        free(buf);

        INF("%s", p);
        sn->ecu = url = ecore_con_url_new(p);
        ecore_con_url_data_set(url, sn);
        ecore_con_url_get(url);
     }

   elm_object_text_set(e->sw.progress, "Searching...");
}

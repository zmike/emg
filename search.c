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
   elm_object_focus_set(e->sw.entry, EINA_TRUE);
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
   Search_Result_Item *sri = elm_object_item_data_get(it);
   Comic_Series *cs;

   cs = comic_series_find(sri->sr->e, sri->name);
   series_view_clear(sri->sr->e);
   if (cs)
     {
        series_view_populate(cs);
        return;
     }
   cs = comic_series_new(sri);
   series_view_title_set(e, cs);
   cs->e->sv.cs = cs;
   series_view_show(e, NULL, NULL);
}

void
search_name_create(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   const char *name;
   char *buf, *buf2, *p, *pp;
   size_t buflen, len = 0;
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
   buflen = strlen(buf);
   EINA_LIST_FOREACH(e->search_providers, l, cb)
     {
        sn = calloc(1, sizeof(Search_Name));
        e->sw.searches = eina_list_append(e->sw.searches, sn);
        sn->identifier = IDENTIFIER_SEARCH_NAME;
        sn->name = eina_stringshare_add(buf);
        sn->namelen = buflen;
        e->sw.running++;
        sn->provider = cb();
        sn->e = e;

        if (sn->provider->replace_str && sn->provider->replace_str[1])
          {
             Eina_Strbuf *sbuf;

             sbuf = eina_strbuf_manage_new(strdup(buf));
             eina_strbuf_replace_all(sbuf, " ", sn->provider->replace_str);
             len = sn->snamelen = eina_strbuf_length_get(sbuf);
             buf2 = eina_strbuf_string_steal(sbuf);
             eina_strbuf_free(sbuf);
             p = NULL;
          }
        else if (sn->provider->replace_str)
          {
             /* avoid realloc */
             buf2 = strdup(buf);
             for (p = pp = strchr(buf2, ' '); pp; pp = strchr(++p, ' '))
               {
                  pp[0] = sn->provider->replace_str[0];
                  p = pp;
               }
             sn->snamelen = sn->namelen;
          }
        /* avoid strlen if possible */
        if (!len)
          {
             if (p)
               {
                  len = p - buf2;
                  len += strlen(p);
               }
             else
               len = sn->namelen;
          }

        p = alloca(len += strlen(sn->provider->search_url) + 1);
        snprintf(p, len, sn->provider->search_url, buf2);

        INF("%s", p);
        sn->ecu = url = ecore_con_url_new(p);
        ecore_con_url_data_set(url, sn);
        if (!ecore_con_url_get(url)) abort();
        free(buf2);
     }
   free(buf);

   elm_object_text_set(e->sw.progress, "Searching...");
}


char *
search_list_text_cb(Search_Result_Item *sri, Evas_Object *obj __UNUSED__, const char *part)
{
   if (!strcmp(part, "elm.text"))
     {
        char *buf;
        size_t size;

        size = sizeof(char) * (sri->namelen + sizeof(" ( chapters)") + 16);
        buf = malloc(size);

        snprintf(buf, size, "%s (%u chapters)", sri->name, sri->total);
        return buf;
     }
   if (!strcmp(part, "elm.text.sub"))
     {
        size_t size;
        Eina_List *l;
        const char *tag;
        char *buf;

        if (!sri->tags) return NULL;
        size = sizeof(char) * (1 + ((eina_list_count(sri->tags) - 1) * 2) + sri->tags_len);
        buf = malloc(size);
        buf[0] = 0;
        EINA_LIST_FOREACH(sri->tags, l, tag)
          {
             strcat(buf, tag);
             if (l->next) strcat(buf, ", ");
          }
        return buf;
     }
   return NULL;
}

Evas_Object *
search_list_pic_cb(Search_Result_Item *sri, Evas_Object *obj, const char *part)
{
   Evas_Object *ic;

   if (strcmp(part, "elm.swallow.end")) return NULL;
   if ((!sri->image) || (!sri->image->buf)) return NULL;
   DBG("%s", sri->name);
   ic = elm_icon_add(obj);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_memfile_set(ic, eina_binbuf_string_get(sri->image->buf), eina_binbuf_length_get(sri->image->buf), NULL, NULL);
   evas_object_show(ic);
   return ic;
}

void
search_view_create(EMG *e, Evas_Object *win)
{
   Evas_Object *box, *progress, *entry, *list;

   box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   e->sw.nf_it = elm_naviframe_item_simple_push(e->nf, box);
   evas_object_show(box);

   e->sw.fr = elm_frame_add(win);
   WEIGHT(e->sw.fr, EVAS_HINT_EXPAND, 0);
   FILL(e->sw.fr);
   elm_box_pack_end(box, e->sw.fr);
   elm_object_text_set(e->sw.fr, "Search");
   elm_frame_autocollapse_set(e->sw.fr, EINA_TRUE);

   e->sw.box = elm_box_add(win);
   EXPAND(e->sw.box);
   FILL(e->sw.box);
   elm_box_horizontal_set(e->sw.box, EINA_TRUE);
   elm_object_content_set(e->sw.fr, e->sw.box);
   evas_object_show(e->sw.box);

   e->sw.progress = progress = elm_progressbar_add(e->win);
   WEIGHT(progress, EVAS_HINT_EXPAND, 0.0);
   FILL(progress);
   elm_box_pack_end(e->sw.box, progress);
   elm_progressbar_value_set(progress, 0.0);
   elm_progressbar_horizontal_set(progress, EINA_TRUE);
   evas_object_show(progress);

   e->sw.entry = entry = elm_entry_add(win);
   WEIGHT(entry, 0.5, EVAS_HINT_EXPAND);
   ALIGN(entry, EVAS_HINT_FILL, 0.5);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
   elm_entry_scrollbar_policy_set(entry, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
   elm_entry_cursor_end_set(entry);
   /* TEMP */
   elm_entry_entry_set(entry, "tower of god");

   elm_entry_select_all(entry);

   elm_box_pack_end(e->sw.box, entry);
   evas_object_show(entry);
   evas_object_smart_callback_add(entry, "activated", (Evas_Smart_Cb)search_name_create, &e);
   evas_object_show(e->sw.fr);

   e->sw.itc.func.text_get = (Elm_Genlist_Item_Text_Get_Cb)search_list_text_cb;
   e->sw.itc.func.content_get  = (Elm_Genlist_Item_Content_Get_Cb)search_list_pic_cb;
   e->sw.itc.func.state_get = NULL;
   e->sw.itc.func.del       = NULL;
   e->sw.list = list = elm_genlist_add(win);
   e->sw.itc.version = ELM_GENLIST_ITEM_CLASS_VERSION;
   EXPAND(list);
   FILL(list);
   elm_genlist_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   elm_box_pack_end(box, list);
   search_name_list_init(e, list);
   evas_object_show(list);
}

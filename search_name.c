#include "emg.h"

static Evas_Object *
_search_name_tooltip_cb(Search_Result *sr, Evas_Object *obj __UNUSED__, Evas_Object *tt, void *it __UNUSED__)
{
   Evas_Object *ic;

   if (!sr->image.buf) return NULL;
   ic = elm_icon_add(tt);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_memfile_set(ic, eina_binbuf_string_get(sr->image.buf), eina_binbuf_length_get(sr->image.buf), NULL, NULL);
   elm_icon_scale_set(ic, 0, 0);
   elm_icon_aspect_fixed_set(ic, EINA_TRUE);
   elm_icon_fill_outside_set(ic, EINA_FALSE);
   evas_object_show(ic);
   return ic;
}

static void
_search_name_pick_cb(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *it)
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

static void
_search_name_realize_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *ev)
{
   elm_object_item_tooltip_content_cb_set(ev, (Elm_Tooltip_Item_Content_Cb)_search_name_tooltip_cb, elm_object_item_data_get(ev), NULL);
}

static void
_search_name_unrealize_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *ev)
{
   elm_object_item_tooltip_unset(ev);
}

void
search_name_free(Search_Name *sn)
{
   Search_Result *sr;
   Eina_Inlist *l;
   unsigned int x = 0;

   if (!sn) return;
   ecore_con_url_free(sn->ecu);
   eina_stringshare_del(sn->name);
   eina_stringshare_del(sn->provider.url);
   EINA_INLIST_FOREACH_SAFE(sn->results, l, sr)
     {
        search_result_free(sr);
        /* safety */
        if (++x == sn->result_count) break;
     }
}

char *
search_name_list_text_cb(Search_Result *sr, Evas_Object *obj __UNUSED__, const char *part)
{
   if (!strcmp(part, "elm.text"))
     {
        char *buf;
        size_t size;

        size = sizeof(char) * (sr->namelen + sizeof(" ( chapters)") + 16);
        buf = malloc(size);

        snprintf(buf, size, "%s (%u chapters)", sr->name, sr->total);
        return buf;
     }
   if (!strcmp(part, "elm.text.sub"))
     {
        size_t size;
        Eina_List *l;
        const char *tag;
        char *buf;

        if (!sr->tags) return NULL;
        size = sizeof(char) * (1 + ((eina_list_count(sr->tags) - 1) * 2) + sr->tags_len);
        buf = malloc(size);
        buf[0] = 0;
        EINA_LIST_FOREACH(sr->tags, l, tag)
          {
             strcat(buf, tag);
             if (l->next) strcat(buf, ", ");
          }
        return buf;
     }
   return NULL;
}

Evas_Object *
search_name_list_pic_cb(Search_Result *sr, Evas_Object *obj, const char *part)
{
   Evas_Object *ic;

   if (strcmp(part, "elm.swallow.end")) return NULL;
   if (!sr->image.buf) return NULL;
   ic = elm_icon_add(obj);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_memfile_set(ic, eina_binbuf_string_get(sr->image.buf), eina_binbuf_length_get(sr->image.buf), NULL, NULL);
   evas_object_show(ic);
   return ic;
}

void
search_name_parser(Search_Name *sn)
{
   sn->provider.data_cb(sn);
}

void
search_name_list_init(EMG *e, Evas_Object *list)
{
   evas_object_smart_callback_add(list, "activated", (Evas_Smart_Cb)_search_name_pick_cb, e);
   evas_object_smart_callback_add(list, "realized", (Evas_Smart_Cb)_search_name_realize_cb, NULL);
   evas_object_smart_callback_add(list, "unrealized", (Evas_Smart_Cb)_search_name_unrealize_cb, NULL);
}

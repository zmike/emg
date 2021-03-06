#include "emg.h"

static Evas_Object *
_search_name_tooltip_cb(Search_Result_Item *sri, Evas_Object *obj __UNUSED__, Evas_Object *tt, void *it __UNUSED__)
{
   Evas_Object *ic;

   if ((!sri->image) || (!sri->image->buf)) return NULL;
   ic = elm_icon_add(tt);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_memfile_set(ic, eina_binbuf_string_get(sri->image->buf), eina_binbuf_length_get(sri->image->buf), NULL, NULL);
   elm_icon_resizable_set(ic, 0, 0);
   elm_icon_aspect_fixed_set(ic, EINA_TRUE);
   elm_icon_fill_outside_set(ic, EINA_FALSE);
   evas_object_show(ic);
   return ic;
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
   if (!sn->results) return;
   EINA_INLIST_FOREACH_SAFE(sn->results, l, sr)
     {
        search_result_free(sr);
        /* safety */
        if (++x == sn->result_count) break;
     }
}

void
search_name_parser(Search_Name *sn)
{
   sn->provider->data_cb(sn);
}

void
search_name_list_init(EMG *e, Evas_Object *list)
{
   evas_object_smart_callback_add(list, "activated", (Evas_Smart_Cb)search_result_pick, e);
   evas_object_smart_callback_add(list, "realized", (Evas_Smart_Cb)_search_name_realize_cb, NULL);
   evas_object_smart_callback_add(list, "unrealized", (Evas_Smart_Cb)_search_name_unrealize_cb, NULL);
}

#include "emg.h"


static void
update_view_list_select(Elm_Object_Item *it, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_naviframe_item_promote(it);
}

static void
_update_view_pick_pers_cb(void *data, Evas_Object *obj, void *event_info)
{
}

static void
_update_view_pick_gen_cb(void *data, Evas_Object *obj, void *event_info)
{
}

static char *
update_view_list_text_cb(Update_Result_Item *uri, Evas_Object *obj __UNUSED__, const char *part)
{
   char buf[512];

   if (strcmp(part, "elm.text")) return NULL;

   /* FIXME: names that include '&' */
   if (uri->ur->vol_set)
     {
        if (uri->ur->num_set)
          snprintf(buf, sizeof(buf), "%s Vol.%u Ch.%g%s%s", uri->series_name, uri->ur->vol, uri->ur->number, uri->chapter_name ? ": " : "", uri->chapter_name ?: "");
        else
          snprintf(buf, sizeof(buf), "%s Vol.%u%s%s", uri->series_name, uri->ur->vol, uri->chapter_name ? ": " : "", uri->chapter_name ?: "");
     }
   else if (uri->ur->num_set)
     snprintf(buf, sizeof(buf), "%s Ch.%g%s%s", uri->series_name, uri->ur->number, uri->chapter_name ? ": " : "", uri->chapter_name ?: "");
   else
     snprintf(buf, sizeof(buf), "%s%s%s", uri->series_name, uri->chapter_name ? ": " : "", uri->chapter_name ?: "");
   
   return strdup(buf);
}

void
update_view_create(EMG *e, Evas_Object *win)
{
   Evas_Object *box, *list;
   int x;
   Evas_Smart_Cb cb[2];

   e->uv.box = box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   e->uv.nf_it = elm_naviframe_item_simple_push(e->nf, box);
   evas_object_show(box);

   e->uv.tb = elm_toolbar_add(win);
   WEIGHT(e->uv.tb, EVAS_HINT_EXPAND, 0);
   FILL(e->uv.tb);
   elm_object_style_set(e->uv.tb, "item_horizontal");
   elm_toolbar_select_mode_set(e->uv.tb, ELM_OBJECT_ALWAYS_SELECT);
   elm_box_pack_end(box, e->uv.tb);
   evas_object_show(e->uv.tb);

   e->uv.nf = elm_naviframe_add(win);
   EXPAND(e->uv.nf);
   FILL(e->uv.nf);
   elm_box_pack_end(box, e->uv.nf);
   evas_object_show(e->uv.nf);

   e->uv.itc.item_style     = NULL;
   e->uv.itc.func.text_get = (Elm_Genlist_Item_Text_Get_Cb)update_view_list_text_cb;
   e->uv.itc.func.content_get  = NULL;
   e->uv.itc.func.state_get = NULL;
   e->uv.itc.func.del       = NULL;
   e->uv.itc.version = ELM_GENLIST_ITEM_CLASS_VERSION;

   cb[0] = (Evas_Smart_Cb)_update_view_pick_gen_cb; /* general */
   cb[1] = (Evas_Smart_Cb)_update_view_pick_pers_cb; /* general */
   for (x = 1; x >= 0; x--)
     {
        e->uv.list[x] = list = elm_genlist_add(win);
        EXPAND(list);
        FILL(list);
        elm_genlist_compress_mode_set(list, EINA_TRUE);
        elm_genlist_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
        evas_object_smart_callback_add(list, "activated", cb[x], e);
        e->uv.uv_nf_it[x] = elm_naviframe_item_simple_push(e->uv.nf, list);
        evas_object_show(list);
     }
   e->uv.uv_tb_it[0] = elm_toolbar_item_append(e->uv.tb, NULL, "General", (Evas_Smart_Cb)update_view_list_select, e->uv.uv_nf_it[0]);
   e->uv.uv_tb_it[1] = elm_toolbar_item_append(e->uv.tb, NULL, "Personal", (Evas_Smart_Cb)update_view_list_select, e->uv.uv_nf_it[1]);
   elm_toolbar_item_selected_set(e->uv.uv_tb_it[0], EINA_TRUE);
}

Update *
update_new(EMG *e)
{
   Update *u;

   u = calloc(1, sizeof(Update));
   u->identifier = IDENTIFIER_UPDATE;
   u->e = e;
   e->uv.updates = eina_list_append(e->uv.updates, u);
   return u;
}

Update_Result *
update_result_add(Update *u)
{
   Update_Result *ur;

   ur = calloc(1, sizeof(Update_Result));
   ur->e = u->e;
   ur->provider = u->provider;
   return ur;
}

void
updates_poll(EMG *e)
{
   Eina_List *l;
   Provider_Init_Cb cb;
   Update *u;

   EINA_LIST_FOREACH(e->update_providers, l, cb)
     {
        u = update_new(e);
        u->provider = cb();
        u->ecu = ecore_con_url_new(u->provider->search_url);
        ecore_con_url_data_set(u->ecu, u);
        if (!ecore_con_url_get(u->ecu)) abort();
     }
}

void
update_view_show(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *event_info)
{
   if (!event_info)
     {
        elm_toolbar_item_selected_set(e->uv.tb_it, EINA_TRUE);
        return;
     }
   if (e->view == EMG_VIEW_UPDATES) return;
   elm_frame_collapse_go(e->sw.fr, EINA_TRUE);
   elm_naviframe_item_promote(e->uv.nf_it);
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
   e->view = EMG_VIEW_UPDATES;
}

void
update_parser(Update *u)
{
   u->provider->data_cb(u);
}

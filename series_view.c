#include "emg.h"
#include <time.h>

#define SERIES_VIEW_POPULATE_COUNT 10

static void
_series_view_pick_cb(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *it)
{
   Comic_Chapter_Item *cci = elm_object_item_data_get(it);

   comic_view_chapter_set(e, cci);
   comic_view_show(e, NULL, NULL);
}

static int
_series_view_compare_cb(Elm_Object_Item *it1, Elm_Object_Item *it2)
{
   Comic_Chapter_Item *a = elm_object_item_data_get(it1), *b = elm_object_item_data_get(it2);
   return a->cc->number - b->cc->number;
}


void
series_view_populate(Comic_Series *cs)
{
   Comic_Chapter_Item *cci = NULL;
   Eina_Inlist *l;
   unsigned int count = 0;

   //DBG("cs=%s", cs->name);
   if (!cs->chapters) return;
   l = cs->populate_job ?: cs->chapters;
   cci = EINA_INLIST_CONTAINER_GET(l, Comic_Chapter_Item);
   for (; cci; cci = comic_chapter_item_next_get(cci))
     {
        if (cci->it)
          elm_genlist_item_update(cci->it);
        else
          {
             cci->it = elm_genlist_item_sorted_insert(cs->e->sv.list, &cs->e->sv.itc, cci, NULL, 0, (Eina_Compare_Cb)_series_view_compare_cb, NULL, NULL);
             //DBG("adding item for cc %g", cci->cc->number);
          }
        if (++count == SERIES_VIEW_POPULATE_COUNT) break;
     }
   if (cci)
     {
        cs->populate_job = EINA_INLIST_GET(cci)->next;
        if (cs->populate_job) ecore_job_add((Ecore_Cb)series_view_populate, cs);
     }
}

char *
series_view_list_text_cb(Comic_Chapter_Item *cci, Evas_Object *obj __UNUSED__, const char *part)
{
   char *buf;
   size_t size;
   struct tm *t;
   char date[128];
   Eina_Bool use_date = EINA_FALSE;

   if (!cci) return NULL;
   if (strcmp(part, "elm.text")) return NULL;

   size = sizeof(char) * (sizeof("Chapter XXXX.XX: (DD/MM/YYYY)") + (cci->name ? strlen(cci->name) : 0));
   buf = malloc(size);

   if (cci->date)
     {
        t = localtime(&cci->date);
        use_date = !!strftime(date, sizeof(date), "%D", t);
     }

   if (cci->cc->decimal)
     snprintf(buf, size, "Chapter %g%s%s%s%s%s%s", cci->cc->number, use_date || cci->name ? ":" : "",
              cci->name ? " " : "", cci->name ?: "", use_date ? " (" : "", use_date ? date : "", use_date ? ")" : "");
   else
     snprintf(buf, size, "Chapter %d%s%s%s%s%s%s", (int)cci->cc->number, use_date || cci->name ? ":" : "",
              cci->name ? " " : "", cci->name ?: "", use_date ? " (" : "", use_date ? date : "", use_date ? ")" : "");
   //INF("CHAPTER NAME SET: %s", buf);
   return buf;
}

void
series_view_clear(EMG *e)
{
   if (e->sv.img) evas_object_del(e->sv.img);
   e->sv.img = NULL;
   elm_genlist_clear(e->sv.list);
   elm_object_text_set(e->sv.title_lbl, "");
   elm_object_text_set(e->sv.desc_lbl, "");
   elm_object_text_set(e->sv.auth_lbl, "");
   elm_object_text_set(e->sv.art_lbl, "");
   elm_object_text_set(e->sv.chap_lbl, "");
   elm_object_text_set(e->sv.year_lbl, "");

}

void
series_view_author_set(EMG *e, Comic_Series *cs)
{
   char *buf;
   size_t size;

   if (e->sv.cs != cs) return;
   size = strlen(cs->author) + sizeof("<b>Author:</b> ");
   buf = alloca(size);
   snprintf(buf, size, "<b>Author:</b> %s", cs->author);
   elm_object_text_set(e->sv.auth_lbl, buf);
}

void
series_view_artist_set(EMG *e, Comic_Series *cs)
{
   char *buf;
   size_t size;

   if (e->sv.cs != cs) return;
   size = strlen(cs->artist) + sizeof("<b>Artist:</b> ");
   buf = alloca(size);
   snprintf(buf, size, "<b>Artist:</b> %s", cs->artist);
   elm_object_text_set(e->sv.art_lbl, buf);
}

void
series_view_chapters_set(EMG *e, Comic_Series *cs)
{
   char *buf;
   size_t size;

   if (e->sv.cs != cs) return;
   size = 16 + sizeof("<b>Chapters:</b> (<i>completed</i>)");
   buf = alloca(size);
   snprintf(buf, size, "<b>Chapters:</b> %u (<i>%s</i>)", cs->total, cs->completed ? "Completed" : "Ongoing");
   elm_object_text_set(e->sv.chap_lbl, buf);
}

void
series_view_year_set(EMG *e, Comic_Series *cs)
{
   char *buf;
   size_t size;

   if (e->sv.cs != cs) return;
   size = 16 + sizeof("<b>Year:</b> ");
   buf = alloca(size);
   snprintf(buf, size, "<b>Year:</b> %u", cs->year);
   elm_object_text_set(e->sv.year_lbl, buf);
}

void
series_view_desc_set(EMG *e, Comic_Series *cs)
{
   if (e->sv.cs != cs) return;
   elm_object_text_set(e->sv.desc_lbl, cs->desc);
}

void
series_view_title_set(EMG *e, Comic_Series *cs)
{
   char *buf;
   size_t size;


   if (e->sv.cs != cs) return;
   if (!cs->alt_name)
     {
        elm_object_text_set(e->sv.title_lbl, cs->name);
        return;
     }
   if (cs->alt_name)
     {
        size = cs->namelen + strlen(cs->alt_name) + sizeof(" (<i></i>)");
        buf = alloca(size);
        snprintf(buf, size, "%s (<i>%s</i>)", cs->name, cs->alt_name);
     }
   else
     buf = strdupa(cs->name);
   elm_object_text_set(e->sv.title_lbl, buf);
}

void
series_view_image_set(EMG *e, Eina_Binbuf *buf)
{
   if (!e->sv.img)
     {
        e->sv.img = elm_icon_add(e->win);
        WEIGHT(e->sv.img, 1, 1);
        FILL(e->sv.img);
        elm_icon_animated_set(e->sv.img, EINA_TRUE);
        elm_icon_aspect_fixed_set(e->sv.img, EINA_TRUE);
        elm_icon_fill_outside_set(e->sv.img, EINA_FALSE);
        elm_box_pack_start(e->sv.box, e->sv.img);
     }
   elm_icon_memfile_set(e->sv.img, eina_binbuf_string_get(buf), eina_binbuf_length_get(buf), NULL, NULL);
   evas_object_show(e->sv.img);
}

void
series_view_show(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *event_info)
{
   if (!event_info)
     {
        elm_toolbar_item_selected_set(e->sv.tb_it, EINA_TRUE);
        return;
     }
   if (e->view == EMG_VIEW_SERIES) return;
   elm_frame_collapse_go(e->sw.fr, EINA_TRUE);
   elm_naviframe_item_promote(e->sv.nf_it);
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
   e->view = EMG_VIEW_SERIES;
}

void
series_view_create(EMG *e, Evas_Object *win)
{
   Evas_Object *scr, *box, *list, *sep;
 
   e->sv.scr = scr = elm_scroller_add(win);
   elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   EXPAND(scr);
   FILL(scr);
   e->sv.nf_it = elm_naviframe_item_simple_push(e->nf, scr);
   evas_object_show(scr);

   e->sv.box = box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   elm_object_content_set(scr, box);
   evas_object_show(box);

   e->sv.title_lbl = elm_label_add(e->win);
   WEIGHT(e->sv.title_lbl, EVAS_HINT_EXPAND, 0);
   elm_box_pack_end(box, e->sv.title_lbl);
   evas_object_show(e->sv.title_lbl);

   sep = elm_separator_add(win);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   WEIGHT(sep, EVAS_HINT_EXPAND, 0);
   elm_box_pack_end(box, sep);
   evas_object_show(sep);

   e->sv.fr = elm_frame_add(win);
   WEIGHT(e->sv.fr, EVAS_HINT_EXPAND, 0);
   FILL(e->sv.fr);
   elm_frame_autocollapse_set(e->sv.fr, EINA_TRUE);
   elm_object_text_set(e->sv.fr, "Info");
   elm_box_pack_end(box, e->sv.fr);
   evas_object_show(e->sv.fr);

   box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   elm_object_content_set(e->sv.fr, box);
   evas_object_show(box);

   /* FIXME: scroller and table here */
   e->sv.desc_lbl = elm_label_add(e->win);
   WEIGHT(e->sv.desc_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e->sv.desc_lbl, EVAS_HINT_FILL, 0.5);
   elm_label_line_wrap_set(e->sv.desc_lbl, ELM_WRAP_WORD);
   elm_box_pack_end(box, e->sv.desc_lbl);
   evas_object_show(e->sv.desc_lbl);

   e->sv.hbox = elm_box_add(win);
   WEIGHT(e->sv.hbox, EVAS_HINT_EXPAND, 0);
   FILL(e->sv.hbox);
   elm_box_horizontal_set(e->sv.hbox, EINA_TRUE);
   elm_box_pack_end(box, e->sv.hbox);
   evas_object_show(e->sv.hbox);

   e->sv.auth_lbl = elm_label_add(e->win);
   WEIGHT(e->sv.auth_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e->sv.auth_lbl, 0, 0.5);
   elm_box_pack_end(e->sv.hbox, e->sv.auth_lbl);
   evas_object_show(e->sv.auth_lbl);

   e->sv.art_lbl = elm_label_add(e->win);
   WEIGHT(e->sv.art_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e->sv.art_lbl, 1, 0.5);
   elm_box_pack_end(e->sv.hbox, e->sv.art_lbl);
   evas_object_show(e->sv.art_lbl);

   e->sv.hbox2 = elm_box_add(win);
   WEIGHT(e->sv.hbox2, EVAS_HINT_EXPAND, 0);
   FILL(e->sv.hbox2);
   elm_box_horizontal_set(e->sv.hbox2, EINA_TRUE);
   elm_box_pack_end(box, e->sv.hbox2);
   evas_object_show(e->sv.hbox2);

   e->sv.chap_lbl = elm_label_add(e->win);
   WEIGHT(e->sv.chap_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e->sv.chap_lbl, 0, 0.5);
   elm_box_pack_end(e->sv.hbox2, e->sv.chap_lbl);
   evas_object_show(e->sv.chap_lbl);

   e->sv.year_lbl = elm_label_add(e->win);
   WEIGHT(e->sv.year_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e->sv.year_lbl, 1, 0.5);
   elm_box_pack_end(e->sv.hbox2, e->sv.year_lbl);
   evas_object_show(e->sv.year_lbl);

   sep = elm_separator_add(win);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   WEIGHT(sep, EVAS_HINT_EXPAND, 0);
   elm_box_pack_end(e->sv.box, sep);
   evas_object_show(sep);

   e->sv.itc.item_style     = NULL;
   e->sv.itc.func.text_get = (Elm_Genlist_Item_Text_Get_Cb)series_view_list_text_cb;
   e->sv.itc.func.content_get  = NULL;
   e->sv.itc.func.state_get = NULL;
   e->sv.itc.func.del       = NULL;
   e->sv.itc.version = ELM_GENLIST_ITEM_CLASS_VERSION;
   e->sv.list = list = elm_genlist_add(e->win);
   EXPAND(list);
   FILL(list);
   elm_genlist_compress_mode_set(list, EINA_TRUE);
   elm_genlist_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   evas_object_smart_callback_add(list, "activated", (Evas_Smart_Cb)_series_view_pick_cb, e);
   elm_box_pack_end(e->sv.box, list);
   evas_object_show(list);
}

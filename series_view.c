#include "emg.h"

#define SERIES_VIEW_POPULATE_COUNT 10

static void
_series_view_pick_cb(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *it)
{
   Comic_Chapter *cc = elm_object_item_data_get(it);

   comic_view_chapter_set(e, cc);  
   comic_view_show(e, NULL, NULL);
}

void
series_view_populate(Comic_Series *cs)
{
   Comic_Chapter *cc = NULL;
   unsigned int count = 0;

   DBG("cs=%s", cs->name);
   EINA_INLIST_FOREACH(cs->populate_job ?: cs->chapters, cc)
     {
        elm_genlist_item_append(cs->e->sv.list, &cs->e->sv.itc, cc, NULL, 0, (Evas_Smart_Cb)NULL, NULL);
        if (++count == SERIES_VIEW_POPULATE_COUNT) break;
     }
   if (cc)
     {
        cs->populate_job = EINA_INLIST_GET(cc)->next;
        if (cs->populate_job) ecore_job_add((Ecore_Cb)series_view_populate, cs);
     }
}

char *
series_view_list_text_cb(Comic_Chapter *cc, Evas_Object *obj __UNUSED__, const char *part)
{
   char *buf;
   size_t size;

   if (strcmp(part, "elm.text")) return NULL;
   if (!cc) return NULL;

   size = sizeof(char) * (sizeof("Chapter XXXX.XX: (DD/MM/YYYY)") + (cc->name ? strlen(cc->name) : 0));
   buf = malloc(size);

   if (cc->decimal)
     snprintf(buf, size, "Chapter %g%s%s%s%s%s%s", cc->number, cc->date || cc->name ? ":" : "",
              cc->name ? " " : "", cc->name ?: "", cc->date ? " (" : "", cc->date ?: "", cc->date ? ")" : "");
   else
     snprintf(buf, size, "Chapter %d%s%s%s%s%s%s", (int)cc->number, cc->date || cc->name ? ":" : "",
              cc->name ? " " : "", cc->name ?: "", cc->date ? " (" : "", cc->date ?: "", cc->date ? ")" : "");
   return buf;
}

void
series_view_clear(EMG *e)
{
   if (e->sv.img) evas_object_del(e->sv.img);
   e->sv.img = NULL;
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
 /* FIXME: use this after label wrapping is fixed
   char *buf;
   if (e->sv.cs != cs) return;

   buf = evas_textblock_text_markup_to_utf8(NULL, cs->desc);
   elm_object_text_set(e->sv.desc_lbl, buf);
   free(buf);
  */
   char *buf;
   size_t size;
   if (e->sv.cs != cs) return;

   buf = evas_textblock_text_markup_to_utf8(NULL, cs->desc);
   size = strlen(buf);
   if (strchr(buf, '\n') || (size < 40))
     elm_object_text_set(e->sv.desc_lbl, buf);
   else
     {
        unsigned int x;
        char *b, *a;

        for (x = 1; x <= size / 40; x++)
          {
             a = strchr(buf + (x * 40), ' ');
             b = buf + (x * 40);
             if (b != buf)
               {
                  while (!isspace(b[0])) b--;
               }
             if (!a)
               b[0] = '\n';
             else if (!b)
               a[0] = '\n';
             else if ((a - buf) > (buf - b))
               b[0] = '\n';
             else
               a[0] = '\n';
          }
        elm_object_text_set(e->sv.desc_lbl, buf);
     }
   free(buf);   
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
        WEIGHT(e->sv.img, EVAS_HINT_EXPAND, 0);
        elm_icon_animated_set(e->sv.img, EINA_TRUE);
        elm_icon_scale_set(e->sv.img, 0, 0);
        elm_icon_aspect_fixed_set(e->sv.img, EINA_TRUE);
        elm_icon_fill_outside_set(e->sv.img, EINA_FALSE);
        elm_box_pack_start(e->sv.box, e->sv.img);
     }
   DBG("changing series view image");
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
series_view_list_init(EMG *e, Evas_Object *list)
{
   evas_object_smart_callback_add(list, "activated", (Evas_Smart_Cb)_series_view_pick_cb, e);
}

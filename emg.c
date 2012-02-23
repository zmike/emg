/*
 * Copyright 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emg.h"
#include <sys/stat.h>
#include <sys/types.h>

int _emg_log_dom = -1;

static EMG e;

static Eina_Bool
_url_progress(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Progress *ev)
{
   int *identifier;
   Search_Name *sn;

   identifier = ecore_con_url_data_get(ev->url_con);
   switch (*identifier)
     {
      case IDENTIFIER_SEARCH_NAME:
        sn = ecore_con_url_data_get(ev->url_con);
        if (sn->e->sw.progress && (ev->down.now > 0.0) && (ev->down.total > 0.0))
          elm_progressbar_value_set(sn->e->sw.progress, ev->down.now / ev->down.total);
        break;
      default:
        break;
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_url_data(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   int *identifier;

   identifier = ecore_con_url_data_get(ev->url_con);
   switch (*identifier)
     {
      case IDENTIFIER_SEARCH_NAME:
        {
           Search_Name *sn;

           sn = ecore_con_url_data_get(ev->url_con);
           if (!sn->buf) sn->buf = eina_strbuf_new();
           eina_strbuf_append_length(sn->buf, (char*)ev->data, ev->size);
           search_name_parser(sn);
           if (sn->done)
             {
                ecore_con_url_free(ev->url_con);
                sn->ecu = NULL;
             }
        }
        break;
      case IDENTIFIER_SEARCH_IMAGE:
      case IDENTIFIER_COMIC_IMAGE:
      case IDENTIFIER_COMIC_PAGE_IMAGE:
        {
           Comic_Image *ci;

           ci = ecore_con_url_data_get(ev->url_con);
           if (!ci->buf) ci->buf = eina_binbuf_new();
           eina_binbuf_append_length(ci->buf, ev->data, ev->size);
           if (*identifier == IDENTIFIER_COMIC_PAGE_IMAGE)
             comic_view_image_update(ci->parent);

           //INF("IMGURL: %s", ci->href);
           break;
        }
      case IDENTIFIER_COMIC_SERIES:
        {
           Comic_Series *cs;

           cs = ecore_con_url_data_get(ev->url_con);
           if (!cs->buf) cs->buf = eina_strbuf_new();
           eina_strbuf_append_length(cs->buf, (char*)ev->data, ev->size);
           comic_series_parser(cs);
           break;
        }
      case IDENTIFIER_COMIC_PAGE:
        {
           Comic_Page *cp;

           cp = ecore_con_url_data_get(ev->url_con);
           if (!cp->buf) cp->buf = eina_strbuf_new();
           eina_strbuf_append_length(cp->buf, (char*)ev->data, ev->size);
           comic_page_parser(cp);
           break;
        }
        
      default:
        break;
     }
   
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_url_complete(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Complete *ev)
{
   unsigned int *identifier;

   identifier = ecore_con_url_data_get(ev->url_con);
   switch (*identifier)
     {
      case IDENTIFIER_SEARCH_IMAGE:
        {
           Comic_Image *ci;
           Search_Result *sr;

           ci = ecore_con_url_data_get(ev->url_con);
           ci->ecu = NULL;
           INF("IMGURL DONE: %s", ci->href);
           sr = ci->parent;
           if (sr->it) elm_genlist_item_update(sr->it);
           break;
        }
      case IDENTIFIER_COMIC_IMAGE:
        {
           Comic_Image *ci;
           Comic_Series *cs;

           ci = ecore_con_url_data_get(ev->url_con);
           ci->ecu = NULL;
           INF("IMGURL DONE: %s", ci->href);
           cs = ci->parent;
           if (cs != cs->e->sv.cs) break;
           series_view_image_set(cs->e, ci->buf);
           break;
        }
      case IDENTIFIER_COMIC_PAGE_IMAGE:
        {
           Comic_Image *ci;
           Comic_Page *cp;

           ci = ecore_con_url_data_get(ev->url_con);
           ci->ecu = NULL;
           cp = ci->parent;
           INF("PAGE %u IMG DONE: %s", cp->number, ci->href);
           comic_view_readahead_ensure(&e);
           comic_view_image_update(cp);
           if (!comic_page_current(cp)) break;
           comic_view_page_set(&e, cp);
           break;
        }
      case IDENTIFIER_SEARCH_NAME:
        {
           Search_Name *sn;

           sn = ecore_con_url_data_get(ev->url_con);
           sn->ecu = NULL;
           elm_object_text_set(sn->e->sw.progress, "Complete");
           elm_progressbar_value_set(sn->e->sw.progress, 100);
           sn->done = EINA_TRUE;
           search_name_parser(sn);
           if (!(--sn->e->sw.running))
             elm_object_disabled_set(sn->e->sw.entry, EINA_FALSE);
           break;
        }
      case IDENTIFIER_COMIC_SERIES:
        {
           Comic_Series *cs;

           cs = ecore_con_url_data_get(ev->url_con);
           cs->done = EINA_TRUE;
           cs->ecu = NULL;
           comic_series_parser(cs);
           eina_strbuf_free(cs->buf);
           cs->buf = NULL;
           if (e.sv.cs == cs)
             {
                series_view_populate(cs);
                elm_genlist_item_selected_set(elm_genlist_first_item_get(e.sv.list), EINA_TRUE);
             }
           elm_object_focus_set(e.sv.list, EINA_TRUE);
           break;
        }
      case IDENTIFIER_COMIC_PAGE:
        {
           Comic_Page *cp;

           cp = ecore_con_url_data_get(ev->url_con);
           cp->ecu = NULL;
           if (cp->buf) eina_strbuf_free(cp->buf);
           cp->buf = NULL;
           comic_view_readahead_ensure(&e);
           INF("PAGE DONE: %s", ecore_con_url_url_get(ev->url_con));
           if (comic_page_current(cp))
             comic_view_page_set(&e, cp);
        }
        break;
      default:
        break;
     }

   ecore_con_url_free(ev->url_con);
   return ECORE_CALLBACK_RENEW;
}

static void
window_key(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Key_Down *ev)
{
   Elm_Object_Item *it;
   switch (e.view)
     {
      case EMG_VIEW_READER:
        if ((!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left")))
          comic_view_page_prev(&e, NULL, NULL);
        else if ((!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right")) || (!strcmp(ev->keyname, "KP_Space")))
          comic_view_page_next(&e, NULL, NULL);
        break;
      case EMG_VIEW_SERIES:
        it = elm_genlist_selected_item_get(e.sv.list);
        if (!it) return;
        comic_view_chapter_set(&e, elm_object_item_data_get(it));
        comic_view_show(&e, NULL, NULL);
        break;
      case EMG_VIEW_SEARCH:
        it = elm_genlist_selected_item_get(e.sw.list);
        if (it)
          search_result_pick(&e, NULL, it);
        else
          search_name_create(&e, NULL, NULL);
      default:
        break;
     }
}

int
main(int argc, char *argv[])
{
   Evas_Object *win, *bg, *box, *box2, *entry, *progress, *list, *sep, *scr, *ic;

   memset(&e.sw, 0, sizeof(Search_Window));

   eina_init();
   ecore_init();
   ecore_con_url_init();
   elm_init(argc, argv);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   _emg_log_dom = eina_log_domain_register("emg", EINA_COLOR_HIGH EINA_COLOR_CYAN);
   eina_log_domain_level_set("emg", EINA_LOG_LEVEL_DBG);

   /* image win */
   e.win = win = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
   elm_win_autodel_set(win, EINA_TRUE);
   elm_win_screen_constrain_set(win, EINA_TRUE);

   evas_object_show(win);

   bg = elm_bg_add(win);
   EXPAND(bg);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   e.box = box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   e.sw.fr = elm_frame_add(win);
   WEIGHT(e.sw.fr, EVAS_HINT_EXPAND, 0);
   FILL(e.sw.fr);
   elm_box_pack_end(box, e.sw.fr);
   elm_object_text_set(e.sw.fr, "Search");
   elm_frame_autocollapse_set(e.sw.fr, EINA_TRUE);

   e.sw.box = elm_box_add(win);
   EXPAND(e.sw.box);
   FILL(e.sw.box);
   elm_box_horizontal_set(e.sw.box, EINA_TRUE);
   elm_object_content_set(e.sw.fr, e.sw.box);
   evas_object_show(e.sw.box);

   e.sw.progress = progress = elm_progressbar_add(e.win);
   WEIGHT(progress, EVAS_HINT_EXPAND, 0.0);
   FILL(progress);
   elm_box_pack_end(e.sw.box, progress);
   elm_progressbar_value_set(progress, 0.0);
   elm_progressbar_horizontal_set(progress, EINA_TRUE);
   evas_object_show(progress);

   e.sw.entry = entry = elm_entry_add(win);
   WEIGHT(entry, 0.5, EVAS_HINT_EXPAND);
   ALIGN(entry, EVAS_HINT_FILL, 0.5);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_cnp_textonly_set(entry, EINA_TRUE);
   elm_entry_scrollbar_policy_set(entry, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
   elm_entry_cursor_end_set(entry);
   /* TEMP */
   elm_entry_entry_set(entry, "one piece");

   elm_entry_select_all(entry);

   elm_box_pack_end(e.sw.box, entry);
   evas_object_show(entry);
   evas_object_smart_callback_add(entry, "activated", (Evas_Smart_Cb)search_name_create, &e);
   evas_object_show(e.sw.fr);

   e.hbox = box2 = elm_box_add(win);
   EXPAND(box2);
   FILL(box2);
   elm_box_horizontal_set(box2, EINA_TRUE);
   elm_box_pack_end(box, box2);
   evas_object_show(box2);

   e.tb = elm_toolbar_add(win);
   WEIGHT(e.tb, 0, EVAS_HINT_EXPAND);
   FILL(e.tb);
   elm_toolbar_shrink_mode_set(e.tb, ELM_TOOLBAR_SHRINK_NONE);
   elm_toolbar_horizontal_set(e.tb, EINA_FALSE);
   elm_toolbar_homogeneous_set(e.tb, EINA_TRUE);
   elm_toolbar_icon_order_lookup_set(e.tb, ELM_ICON_LOOKUP_FDO);
   elm_toolbar_always_select_mode_set(e.tb, EINA_TRUE);
   {
      e.sw.tb_it = elm_toolbar_item_append(e.tb, "system-search", "Search Results", (Evas_Smart_Cb)search_view_show, &e);
      e.sv.tb_it = elm_toolbar_item_append(e.tb, NULL, "Current Series", (Evas_Smart_Cb)series_view_show, &e);
      e.cv.tb_it = elm_toolbar_item_append(e.tb, "view-presentation", "Reader", (Evas_Smart_Cb)comic_view_show, &e);
   }
   elm_box_pack_end(box2, e.tb);
   evas_object_show(e.tb);

   sep = elm_separator_add(win);
   WEIGHT(sep, 0, EVAS_HINT_EXPAND);
   elm_box_pack_end(box2, sep);
   evas_object_show(sep);

   e.nf = elm_naviframe_add(win);
   EXPAND(e.nf);
   FILL(e.nf);
   elm_box_pack_end(box2, e.nf);
   evas_object_show(e.nf);

   e.cv.nf = elm_naviframe_add(win);
   EXPAND(e.cv.nf);
   FILL(e.cv.nf);
   e.cv.nf_it = elm_naviframe_item_simple_push(e.nf, e.cv.nf);
   evas_object_show(e.cv.nf);

   e.cv.prev = elm_button_add(win);
   FILL(e.cv.prev);
   ic = elm_icon_add(win);
   FILL(ic);
   elm_icon_standard_set(ic, "go-previous");
   evas_object_show(ic);
   elm_object_part_content_set(e.cv.prev, "icon", ic);
   evas_object_smart_callback_add(e.cv.prev, "clicked", (Evas_Smart_Cb)comic_view_page_prev, &e);

   e.cv.next = elm_button_add(win);
   FILL(e.cv.next);
   ic = elm_icon_add(win);
   FILL(ic);
   elm_icon_standard_set(ic, "go-next");
   evas_object_show(ic);
   elm_object_part_content_set(e.cv.next, "icon", ic);
   evas_object_smart_callback_add(e.cv.next, "clicked", (Evas_Smart_Cb)comic_view_page_next, &e);

   e.sv.scr = scr = elm_scroller_add(win);
   elm_scroller_bounce_set(scr, EINA_FALSE, EINA_FALSE);
   elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   EXPAND(scr);
   FILL(scr);
   e.sv.nf_it = elm_naviframe_item_simple_push(e.nf, scr);
   evas_object_show(scr);

   e.sv.box = box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   elm_object_content_set(scr, box);
   evas_object_show(box);

   e.sv.title_lbl = elm_label_add(e.win);
   WEIGHT(e.sv.title_lbl, EVAS_HINT_EXPAND, 0);
   elm_box_pack_end(box, e.sv.title_lbl);
   evas_object_show(e.sv.title_lbl);

   sep = elm_separator_add(win);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   WEIGHT(sep, EVAS_HINT_EXPAND, 0);
   elm_box_pack_end(box, sep);
   evas_object_show(sep);

   e.sv.fr = elm_frame_add(win);
   WEIGHT(e.sv.fr, EVAS_HINT_EXPAND, 0);
   FILL(e.sv.fr);
   elm_frame_autocollapse_set(e.sv.fr, EINA_TRUE);
   elm_object_text_set(e.sv.fr, "Info");
   elm_box_pack_end(box, e.sv.fr);
   evas_object_show(e.sv.fr);

   box = elm_box_add(win);
   EXPAND(box);
   FILL(box);
   elm_object_content_set(e.sv.fr, box);
   evas_object_show(box);

   e.sv.desc_lbl = elm_label_add(e.win);
   WEIGHT(e.sv.desc_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e.sv.desc_lbl, EVAS_HINT_FILL, 0.5);
   elm_label_line_wrap_set(e.sv.desc_lbl, ELM_WRAP_WORD);
   elm_box_pack_end(box, e.sv.desc_lbl);
   evas_object_show(e.sv.desc_lbl);

   e.sv.hbox = elm_box_add(win);
   WEIGHT(e.sv.hbox, EVAS_HINT_EXPAND, 0);
   FILL(e.sv.hbox);
   elm_box_horizontal_set(e.sv.hbox, EINA_TRUE);
   elm_box_pack_end(box, e.sv.hbox);
   evas_object_show(e.sv.hbox);

   e.sv.auth_lbl = elm_label_add(e.win);
   WEIGHT(e.sv.auth_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e.sv.auth_lbl, 0, 0.5);
   elm_box_pack_end(e.sv.hbox, e.sv.auth_lbl);
   evas_object_show(e.sv.auth_lbl);

   e.sv.art_lbl = elm_label_add(e.win);
   WEIGHT(e.sv.art_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e.sv.art_lbl, 1, 0.5);
   elm_box_pack_end(e.sv.hbox, e.sv.art_lbl);
   evas_object_show(e.sv.art_lbl);

   e.sv.hbox2 = elm_box_add(win);
   WEIGHT(e.sv.hbox2, EVAS_HINT_EXPAND, 0);
   FILL(e.sv.hbox2);
   elm_box_horizontal_set(e.sv.hbox2, EINA_TRUE);
   elm_box_pack_end(box, e.sv.hbox2);
   evas_object_show(e.sv.hbox2);

   e.sv.chap_lbl = elm_label_add(e.win);
   WEIGHT(e.sv.chap_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e.sv.chap_lbl, 0, 0.5);
   elm_box_pack_end(e.sv.hbox2, e.sv.chap_lbl);
   evas_object_show(e.sv.chap_lbl);

   e.sv.year_lbl = elm_label_add(e.win);
   WEIGHT(e.sv.year_lbl, EVAS_HINT_EXPAND, 0);
   ALIGN(e.sv.year_lbl, 1, 0.5);
   elm_box_pack_end(e.sv.hbox2, e.sv.year_lbl);
   evas_object_show(e.sv.year_lbl);

   sep = elm_separator_add(win);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   WEIGHT(sep, EVAS_HINT_EXPAND, 0);
   elm_box_pack_end(e.sv.box, sep);
   evas_object_show(sep);

   e.sv.itc.item_style     = "default";
   e.sv.itc.func.text_get = (Elm_Genlist_Item_Text_Get_Cb)series_view_list_text_cb;
   e.sv.itc.func.content_get  = NULL;
   e.sv.itc.func.state_get = NULL;
   e.sv.itc.func.del       = NULL;
   e.sv.list = list = elm_genlist_add(e.win);
   EXPAND(list);
   FILL(list);
   elm_genlist_bounce_set(list, EINA_FALSE, EINA_FALSE);
   elm_genlist_compress_mode_set(list, EINA_TRUE);
   elm_genlist_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   series_view_list_init(&e, list);
   elm_box_pack_end(e.sv.box, list);
   evas_object_show(list);

   e.sw.itc.item_style     = "double_label";
   e.sw.itc.func.text_get = (Elm_Genlist_Item_Text_Get_Cb)search_name_list_text_cb;
   e.sw.itc.func.content_get  = (Elm_Genlist_Item_Content_Get_Cb)search_name_list_pic_cb;
   e.sw.itc.func.state_get = NULL;
   e.sw.itc.func.del       = NULL;
   e.sw.list = list = elm_genlist_add(e.win);
   EXPAND(list);
   FILL(list);
   elm_genlist_bounce_set(list, EINA_FALSE, EINA_FALSE);
   elm_genlist_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   e.sw.nf_it = elm_naviframe_item_simple_push(e.nf, list);
   search_name_list_init(&e, list);
   evas_object_show(list);

   elm_object_focus_set(entry, EINA_TRUE);

   ecore_event_handler_add(ECORE_CON_EVENT_URL_PROGRESS, (Ecore_Event_Handler_Cb)_url_progress, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_url_data, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_url_complete, NULL);

   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)window_key, &e);
   {
      Evas *evas;
      Evas_Modifier_Mask ctrl, shift, alt;
      evas = evas_object_evas_get(win);
      ctrl = evas_key_modifier_mask_get(evas, "Control");
      shift = evas_key_modifier_mask_get(evas, "Shift");
      alt = evas_key_modifier_mask_get(evas, "Alt");
      1 | evas_object_key_grab(win, "Return", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
      1 | evas_object_key_grab(win, "KP_Enter", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
   }

   evas_object_resize(win, 640, 712);
   elm_win_center(win, EINA_TRUE, EINA_TRUE);

   e.providers = eina_list_append(e.providers, mangareader_search_init_cb);
   search_view_show(&e, NULL, NULL);

   elm_run();
   elm_shutdown();

   return 0;
}

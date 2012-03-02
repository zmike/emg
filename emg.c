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
   double pct;

   identifier = ecore_con_url_data_get(ev->url_con);
   switch (*identifier)
     {
      case IDENTIFIER_SEARCH_NAME:
        sn = ecore_con_url_data_get(ev->url_con);
        pct = elm_progressbar_value_get(sn->e->sw.progress);
        pct -= sn->pct;
        if (sn->e->sw.progress && (ev->down.now > 0.0) && (ev->down.total > 0.0))
          {
             sn->pct = ev->down.now / ev->down.total;
             elm_progressbar_value_set(sn->e->sw.progress, pct + sn->pct);
          }
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
        }
        break;
      case IDENTIFIER_UPDATE:
        {
           Update *u;

           u = ecore_con_url_data_get(ev->url_con);
           if (!u->buf) u->buf = eina_strbuf_new();
           eina_strbuf_append_length(u->buf, (char*)ev->data, ev->size);
        }
        break;
      case IDENTIFIER_SEARCH_IMAGE:
      case IDENTIFIER_COMIC_SERIES_IMAGE:
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
      case IDENTIFIER_COMIC_SERIES_DATA:
        {
           Comic_Series_Data *csd;

           csd = ecore_con_url_data_get(ev->url_con);
           if (!csd->buf) csd->buf = eina_strbuf_new();
           eina_strbuf_append_length(csd->buf, (char*)ev->data, ev->size);
           break;
        }
      case IDENTIFIER_COMIC_PAGE:
        {
           Comic_Page *cp;

           cp = ecore_con_url_data_get(ev->url_con);
           if (!cp->buf) cp->buf = eina_strbuf_new();
           eina_strbuf_append_length(cp->buf, (char*)ev->data, ev->size);
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

   DBG("transfer completed: %d", ev->status);
   identifier = ecore_con_url_data_get(ev->url_con);
   switch (*identifier)
     {
      case IDENTIFIER_SEARCH_IMAGE:
        {
           Comic_Image *ci;
           Search_Result *sr;

           ci = ecore_con_url_data_get(ev->url_con);
           ci->ecu = NULL;
           sr = ci->parent;
           if (ev->status == 200)
             {
                INF("IMGURL DONE: %s", ci->href);
                search_result_item_update(sr);
             }
           else
             {
                WRN("IMGURL FAILED: %s", ci->href);
                if (ci->buf) eina_binbuf_free(ci->buf);
                ci->buf = NULL;
                search_result_image_fetch(sr);
             }
           break;
        }
      case IDENTIFIER_COMIC_SERIES_IMAGE:
        {
           Comic_Image *ci;
           Comic_Series_Data *csd;

           ci = ecore_con_url_data_get(ev->url_con);
           ci->ecu = NULL;
           csd = ci->parent;
           if (ev->status == 200)
             {
                INF("IMGURL DONE: %s", ci->href);
                if (csd->cs != csd->cs->e->sv.cs) break;
                  series_view_image_set(csd->cs->e, ci->buf);
             }
           else
             {
                WRN("IMGURL FAILED: %s", ci->href);
                if (ci->buf) eina_binbuf_free(ci->buf);
                ci->buf = NULL;
                comic_series_image_fetch(csd);
             }


           break;
        }
      case IDENTIFIER_COMIC_PAGE_IMAGE:
        {
           Comic_Image *ci;
           Comic_Page *cp;

           ci = ecore_con_url_data_get(ev->url_con);
           ci->ecu = NULL;
           cp = ci->parent;
           if (ev->status == 200)
             {
                INF("PAGE %g:%u IMG DONE (%d bytes): %s", cp->cc->number, cp->number, ecore_con_url_received_bytes_get(ev->url_con), ci->href);
                comic_view_image_update(cp);
             }
           else
             {
                WRN("PAGE %g:%u IMG FAILED (code %d): %s", cp->cc->number, cp->number, ev->status, ci->href);
                if (ci->buf) eina_binbuf_free(ci->buf);
                ci->buf = NULL;
             }
           comic_view_readahead_ensure(&e);
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
           search_name_parser(sn);
           sn->done = EINA_TRUE;
           if (!(--sn->e->sw.running))
             elm_object_disabled_set(sn->e->sw.entry, EINA_FALSE);
           if (sn->buf) eina_strbuf_free(sn->buf);
           sn->buf = NULL;
           break;
        }
      case IDENTIFIER_UPDATE:
        {
           Update *u;

           u = ecore_con_url_data_get(ev->url_con);
           u->ecu = NULL;
           u->done = EINA_TRUE;
           update_parser(u);
           eina_strbuf_free(u->buf);
           u->buf = NULL;
           break;
        }
      case IDENTIFIER_COMIC_SERIES_DATA:
        {
           Comic_Series_Data *csd;

           csd = ecore_con_url_data_get(ev->url_con);
           csd->done = EINA_TRUE;
           csd->ecu = NULL;
           comic_series_parser(csd);
           eina_strbuf_free(csd->buf);
           csd->buf = NULL;
           if (e.sv.cs == csd->cs)
             {
                Elm_Object_Item *it;
                csd->cs->populate_job = csd->cs->chapters;
                series_view_populate(csd->cs);
                it = elm_genlist_first_item_get(e.sv.list);
                if (it)
                  elm_genlist_item_selected_set(it, EINA_TRUE);
             }
           elm_object_focus_set(e.sv.list, EINA_TRUE);
           break;
        }
      case IDENTIFIER_COMIC_PAGE:
        {
           Comic_Page *cp;

           cp = ecore_con_url_data_get(ev->url_con);
           cp->ecu = NULL;
           comic_page_parser(cp);
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

static void
_win_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_unref(e.cv.prev);
   evas_object_unref(e.cv.next);
   ecore_main_loop_quit();
}

int
main(int argc, char *argv[])
{
   Evas_Object *win, *bg, *box, *box2, *sep;

   memset(&e.sw, 0, sizeof(Search_Window));

   eina_init();
   ecore_init();
   ecore_con_url_init();
   elm_init(argc, argv);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_scroll_bounce_enabled_set(EINA_FALSE);
   _emg_log_dom = eina_log_domain_register("emg", EINA_COLOR_HIGH EINA_COLOR_CYAN);
   eina_log_domain_level_set("emg", EINA_LOG_LEVEL_DBG);

   /* image win */
   e.win = win = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
   evas_object_smart_callback_add(win, "delete,request", _win_del, NULL);
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
      e.uv.tb_it = elm_toolbar_item_append(e.tb, NULL, "Recently Updated", (Evas_Smart_Cb)update_view_show, &e);
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

   comic_view_create(&e, win);

   series_view_create(&e, win);
   search_view_create(&e, win);
   update_view_create(&e, win);
   e.sw.itc.item_style = "double_label";

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

   elm_win_screen_constrain_set(win, EINA_TRUE);
   evas_object_resize(win, 640, 712);
   elm_win_center(win, EINA_TRUE, EINA_TRUE);

   e.search_providers = eina_list_append(e.search_providers, mangareader_search_init_cb);
   e.search_providers = eina_list_append(e.search_providers, batoto_search_init_cb);
   e.update_providers = eina_list_append(e.update_providers, mangaupdates_update_init_cb);
   e.update_providers = eina_list_append(e.update_providers, batoto_update_init_cb);
   update_view_show(&e, NULL, NULL);
   updates_poll(&e);

   elm_run();
   elm_shutdown();

   return 0;
}

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
   if (!event_info)
     {
        elm_toolbar_item_selected_set(e->sw.tb_it, EINA_TRUE);
        return;
     }
   elm_frame_collapse_go(e->sw.fr, EINA_FALSE);
   elm_naviframe_item_simple_promote(e->nf, e->sw.list);
}

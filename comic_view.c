#include "emg.h"

void
comic_view_show(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *event_info __UNUSED__)
{
   if (!event_info)
     {
        elm_toolbar_item_selected_set(e->cv.tb_it, EINA_TRUE);
        return;
     }
   elm_frame_collapse_go(e->sw.fr, EINA_TRUE);
   elm_naviframe_item_simple_promote(e->nf, e->cv.img);
}

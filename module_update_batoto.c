#include "module_update_batoto.h"
#include <Azy.h>

static void batoto_data_cb(Update *u);

static Comic_Provider update_provider =
{
   .url = BATOTO_URL,
   .search_url = BATOTO_UPDATE_URL,
   .priority = BATOTO_PROVIDER_PRIORITY,
   .data_cb = (Provider_Data_Cb)batoto_data_cb
};

static const char *
_chnum_parser(Update_Result *ur, const char *vol)
{
   const char *pp, *end;

   if ((vol[0] == 'v') || (vol[0] == 'V'))
     {
        pp = strchr(vol, '.');
        if (!pp) return NULL;

        errno = 0;
        ur->vol = strtoul(pp + 1, (char**)&end, 10);
        if (errno)
          ERR("%s", pp + 1);
        else
          ur->vol_set = EINA_TRUE;

        if (!end) return NULL;
     }
   else
     end = vol;
   while (end[0] == ' ') end++;
   if (end && ((end[0] == 'c') || (end[0] == 'C')))
     {
        pp = strchr(end, '.');
        if (!pp) return NULL;
        errno = 0;
        ur->number = strtod(pp + 1, (char**)&end);
        if (errno)
          ERR("%s", end);
        else
          ur->num_set = EINA_TRUE;
     }
   return end;
}

static void
batoto_data_cb(Update *u)
{
   Azy_Net *net;
   Azy_Content *content;
   Azy_Rss *rss;
   Azy_Rss_Item *it;
   size_t size;
   Eina_List *l;

   size = eina_strbuf_length_get(u->buf);

   net = azy_net_buffer_new(eina_strbuf_string_steal(u->buf), size, AZY_NET_TRANSPORT_XML, EINA_TRUE);
   content = azy_content_new(NULL);
   if (!azy_content_deserialize(content, net)) abort();

   rss = azy_content_return_get(content);
   if (!rss) abort();

   azy_net_free(net);
   azy_content_free(content);

   EINA_LIST_FOREACH((Eina_List*)azy_rss_items_get(rss), l, it)
     {
        const char *s, *p, *pp;
        Update_Result *ur;

        s = azy_rss_item_title_get(it);
        p = strstr(s, " - ");
        if (!p)
          {
             WRN("Skipping update %s", s);
             continue;
          }
        if (strncmp(p + 3, "English", 7))
          {
             pp = strstr(p + 3, " - ");
             if (pp && (!strncmp(pp + 3, "English", 7)))
               p = pp;
             else
               {
                  DBG("Skipping non-english update %s", s);
                  continue;
               }
          }
        ur = update_result_add(u);
        ur->series_namelen = (unsigned int)(p - s);
        ur->series_name = eina_stringshare_add_length(s, ur->series_namelen);
        p += 13;
        pp = strchr(p, ':');
        if (!pp)
          {
             _chnum_parser(ur, p);
             update_result_item_result_add(ur);
             continue;
          }
        pp = _chnum_parser(ur, p);
        if (pp)
          ur->chapter_name = eina_stringshare_add(pp + 1);
        update_result_item_result_add(ur);
     }
   azy_rss_free(rss);
}

Comic_Provider *
batoto_update_init_cb(void)
{
   azy_init();
   return &update_provider;
}

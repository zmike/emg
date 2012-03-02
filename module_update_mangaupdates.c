#include "module_update_mangaupdates.h"
#include <Azy.h>

static void mangaupdates_data_cb(Update *u);

static Comic_Provider update_provider =
{
   .url = MANGAUPDATES_URL,
   .search_url = MANGAUPDATES_UPDATE_URL,
   .priority = MANGAUPDATES_PROVIDER_PRIORITY,
   .data_cb = (Provider_Data_Cb)mangaupdates_data_cb
};


static void
mangaupdates_data_cb(Update *u)
{
   Azy_Net *net;
   Azy_Content *content;
   Azy_Rss *rss;
   Azy_Rss_Item *it;
   size_t size;
   Eina_List *l;

   if (!u->done) return;
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
        const char *s, *p, *group = NULL;
        Update_Result *ur;

        s = azy_rss_item_title_get(it);
        if (s[0] == '[')
          {
             group = strchr(s + 1, ']');
             p = group + 1;
          }
        else
          p = s;

        p = strchr(p, '.');
        if (!p) continue; /* nobody cares */

        ur = update_result_add(u);
        {
           const char *pp;

           pp = memrchr(s, ' ', p - s);
           while (pp[-1] == ' ') pp--;
           ur->namelen = pp - (group ? group + 1 : s);
           ur->name = eina_stringshare_add_length(group ? group + 1 : s, ur->namelen);
        }
        if (group)
          ur->group_name = eina_stringshare_add_length(s + 1, group - (s + 1));
        if ((p[-1] == 'v') || (p[-1] == 'V'))
          {
             ur->vol = strdup(p - 1);
             p = strchr(p + 1, '.');
          }
        if (p && ((p[-1] == 'c') || (p[-1] == 'C')))
          {
             if (!ur->vol)
               ur->vol = strdup(p - 1);
             ur->number = strtod(p + 1, NULL);
          }
        if (ur->vol)
          {
             char *pp;

             pp = strrchr(ur->vol, ' ');
             for (; pp && (pp[0] == ' '); pp--)
               {
                  if (pp[1] == 0)
                    pp[0] = 0;
               }
          }
        update_result_item_result_add(ur);
     }
   azy_rss_free(rss);
}

Comic_Provider *
mangaupdates_update_init_cb(void)
{
   azy_init();
   return &update_provider;
}

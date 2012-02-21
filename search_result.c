#include "emg.h"

void
search_result_free(Search_Result *sr)
{
   const char *s;

   if (!sr) return;
   INF("freeing %s", sr->name);
   eina_stringshare_del(sr->name);
   free(sr->href);
   EINA_LIST_FREE(sr->tags, s)
      eina_stringshare_del(s);
   eina_stringshare_del(sr->image.imgurl);
   if (sr->image.buf) eina_binbuf_free(sr->image.buf);
   eina_stringshare_del(sr->provider_url);
   free(sr);
}

Search_Result *
search_result_add(Search_Name *sn)
{
   Search_Result *sr;

   sr = calloc(1, sizeof(Search_Result));
   sr->search = (unsigned int*)sn;
   sr->e = sn->e;
   sr->image.identifier = IDENTIFIER_SEARCH_IMAGE;
   sr->provider_url = eina_stringshare_ref(sn->provider.url);
   sr->image.parent = sr;
   sn->results = eina_inlist_append(sn->results, EINA_INLIST_GET(sr));
   sn->result_count++;
   return sr;
}

void
search_result_tag_add(Search_Result *sr, const char *index_start, const char *tag)
{
   const char *t;

   t = eina_stringshare_add_length((char*)index_start, tag - index_start);
   INF("tag=%s", t);
   sr->tags = eina_list_append(sr->tags, t);
   sr->tags_len += (tag - index_start);
}

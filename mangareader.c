#include "mangareader.h"

void
mangareader_search_name_cb(Search_Name *sn, Ecore_Con_Event_Url_Data *ev)
{
   if ((!sn->idx[0]) && (!sn->idx[1]))
     sn->idx[0] = sn->provider.search_index + (sn->provider.search_name_count * sn->namelen);
   DBG("(idx=%u,ev->size=%d)", sn->idx[0], ev->size);
   /* discard unneeded bytes, hooray */
   for (; (sn->idx[1] < sizeof(sn->provider.index_start)) && sn->provider.index_start[sn->idx[1]]; sn->idx[1]++)
     {
        Search_Result *sr = NULL;
        unsigned char *p, *index_start;

        DBG("(idx=%u,ev->size=%d)", sn->idx[0], ev->size);
        if (sn->idx[0] + sn->provider.index_start[sn->idx[1]] + 8 > (unsigned int)ev->size)
          {
             sn->idx[0] -= ev->size;
             /*
             char *buf;
             buf = strndupa((char*)ev->data, ev->size);
             DBG("%s", buf);
             */
             return;
          }
        sn->idx[0] += sn->provider.index_start[sn->idx[1]];
        index_start = ev->data + sn->idx[0];
        if (!memcmp(index_start, "adfooter", 8))
          {
             sn->done = EINA_TRUE;
             return;
          }
        if (sn->provider.index_char[sn->idx[1]])
          p = memchr(index_start, sn->provider.index_char[sn->idx[1]], ev->size - sn->idx[0]);
        switch (sn->idx[1])
          {
           case 0: /* result image (thumb) */
             sr = search_result_add(sn);
             sr->image.imgurl = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("imgurl=%s", sr->image.imgurl);
             {
                Ecore_Con_Url *url = NULL;
                url = ecore_con_url_new(sr->image.imgurl);
                ecore_con_url_data_set(url, &sr->image);
                ecore_con_url_get(url);
             }
             break;
           case 1: /* result link */
             sr = EINA_INLIST_CONTAINER_GET(sn->results->last, Search_Result);
             sr->href = strndup((char*)index_start, p - index_start);
             INF("href=%s", sr->href);
             break;
           case 2: /* result name */
             sr = EINA_INLIST_CONTAINER_GET(sn->results->last, Search_Result);
             sr->name = eina_stringshare_add_length((char*)index_start, p - index_start);
             sr->namelen = p - index_start;
             INF("name=%s", sr->name);
             sr->it = elm_genlist_item_append(sn->e->sw.list, &sn->e->sw.itc, sr, NULL, 0, (Evas_Smart_Cb)NULL, sr);
             break;
           case 3: /* total chapters */
             sr = EINA_INLIST_CONTAINER_GET(sn->results->last, Search_Result);
             errno = 0;
             sr->total = strtoul((char*)index_start, NULL, 10);
             INF("chapters=%u", sr->total);
             if (errno) {/* FIXME */}
             break;
           case 4: /* result tags */
             {
                unsigned char *tag;
                sr = EINA_INLIST_CONTAINER_GET(sn->results->last, Search_Result);
                if (!isalnum(index_start[0]))
                  index_start++;
                while ((tag = memchr(index_start, ',', p - index_start)))
                  {
                     if (index_start[0] == ' ') index_start++;
                     search_result_tag_add(sr, index_start, tag);
                     if (eina_list_count(sr->tags) > 20) break;
                     index_start = tag + 1;
                  }
                if (index_start[0] == ' ') index_start++;
                search_result_tag_add(sr, index_start, p);
                if (sr->it) elm_genlist_item_update(sr->it);
                break;
             }
           default:
             sn->idx[1] = -1;
             continue;
          }
        sn->idx[0] = p - ev->data;
     }
}

void
mangareader_comic_series_data_cb(Comic_Series *cs, Ecore_Con_Event_Url_Data *ev)
{
   if ((!cs->idx[0]) && (!cs->idx[1]))
     cs->idx[0] = cs->provider.search_index + (MANGAREADER_SERIES_INDEX_NAME_COUNT * cs->namelen);
   DBG("(idx=%u,ev->size=%d)", cs->idx[0], ev->size);
   /* discard unneeded bytes, hooray */
   for (; (cs->idx[1] < sizeof(cs->provider.index_start)) && (cs->provider.index_start[cs->idx[1]] || cs->provider.index_char[cs->idx[1]]); cs->idx[1]++)
     {
        unsigned char *p, *index_start;
        unsigned int jump = 0;

        DBG("(idx=%u,ev->size=%d)", cs->idx[0], ev->size);
        if (cs->idx[1] == 1) jump = MANGAREADER_SERIES_INDEX_POST_IMAGE_NAME_COUNT * cs->namelen;
        if (cs->idx[0] + cs->provider.index_start[cs->idx[1]] + jump > (unsigned int)ev->size)
          {
             cs->idx[0] -= ev->size;
             /*
             char *buf;
             buf = strndupa((char*)ev->data, ev->size);
             DBG("%s", buf);
             */
             return;
          }
        cs->idx[0] += cs->provider.index_start[cs->idx[1]] + jump;
        index_start = ev->data + cs->idx[0];
        /*
        if (!memcmp(index_start, "adfooter", 8))
          {
             cs->done = EINA_TRUE;
             return;
          }
        */
        if (cs->provider.index_char[cs->idx[1]])
          p = memchr(index_start, cs->provider.index_char[cs->idx[1]], ev->size - cs->idx[0]);
        switch (cs->idx[1])
          {
           case 0: /* series image */
             cs->image.imgurl = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("imgurl=%s", cs->image.imgurl);
             {
                Ecore_Con_Url *url = NULL;
                url = ecore_con_url_new(cs->image.imgurl);
                ecore_con_url_data_set(url, &cs->image);
                ecore_con_url_get(url);
             }
             break;
           case 1: /* alt name */
             if (p - index_start > 0)
               {
                  cs->alt_name = eina_stringshare_add_length((char*)index_start, p - index_start);
                  INF("alt_name=%s", cs->alt_name);
                  series_view_title_set(cs->e, cs);
               }
             break;
           case 2: /* year */
             errno = 0;
             cs->year = strtoul((char*)index_start, NULL, 10);
             INF("year=%u", cs->total);
             series_view_year_set(cs->e, cs);
             if (errno) {/* FIXME */}
             break;
           case 3: /* completed or not */
             cs->completed = memcmp(index_start, "Ongoing", 7);
             series_view_chapters_set(cs->e, cs);
             break;
           case 4: /* author */
             if (p - index_start > 0)
               {
                  cs->author = eina_stringshare_add_length((char*)index_start, p - index_start);
                  INF("author=%s", cs->author);
                  series_view_author_set(cs->e, cs);
               }
             break;
           case 5: /* artist */
             if (p - index_start > 0)
               {
                  cs->artist = eina_stringshare_add_length((char*)index_start, p - index_start);
                  INF("artist=%s", cs->artist);
                  series_view_artist_set(cs->e, cs);
               }
             break;
           case 6:
             cs->buf = eina_strbuf_new();
             break;
           default:
             eina_strbuf_append_length(cs->buf, (char*)index_start, ev->size - cs->idx[0]);
             cs->idx[1]++;
             return;
          }
        cs->idx[0] = p - ev->data;
     }
   eina_strbuf_append_length(cs->buf, (char*)ev->data, ev->size);
}

static void
mangareader_comic_series_init_cb(Comic_Series *cs)
{
   const char *base, *buf, *p;
   size_t size;

   base = buf = eina_strbuf_string_get(cs->buf);
   size = eina_strbuf_length_get(cs->buf);
   p = strstr(buf, "readmangasum");
   if (!p) abort(); /* FIXME */
   if (size < (unsigned int)((p + 46 + cs->namelen) - base)) abort(); /* FIXME */
   buf = p + 46 + cs->namelen;
   p = strchr(buf, '<');
   if (!p) abort(); /* FIXME */
   cs->desc = strndup(buf, p - buf);
   series_view_desc_set(cs->e, cs);
   INF("desc=%s", cs->desc);
   if ((int)size < (p + 195) - base) abort(); /* FIXME */
   buf = p + 195;
   /* FIXME: size */
   while (1)
     {
        Comic_Chapter *cc;
        char *pp;
        
        if (buf[0] != '/') break;
        p = strchr(buf, '"');
        if (!p) abort(); /* FIXME */
        cc = comic_chapter_new(cs);
        cc->href = eina_stringshare_add_length(buf, p - buf);
        buf = p + cs->namelen + 3;
        cc->number = strtod(buf, &pp);
        if (pp - buf > 2)
          {
             buf = memchr(buf, '.', pp - buf);
             if (buf) cc->decimal = EINA_TRUE;
          }
        buf = pp + 7;
        p = strchr(buf, '<');
        if (!p) abort(); /* FIXME */
        if (p - buf > 1)
          cc->name = eina_stringshare_add_length(buf, p - buf);
        buf = p + 10;
        p = strchr(buf, '<');
        if (!p) abort(); /* FIXME */
        cc->date = eina_stringshare_add_length(buf, p - buf);
        buf = p + 63;
        INF("chapter: %g - %s", cc->number, cc->name);
     }
   if (cs->current)
     ecore_job_add((Ecore_Cb)series_view_populate, cs);
   eina_strbuf_free(cs->buf);
   cs->buf = NULL;
}

void
mangareader_search_init_cb(Search_Name *sn)
{
   MANGAREADER_SEARCH_SETUP(sn);
}

void
mangareader_series_init_cb(Comic_Series *cs)
{
   cs->provider.url = eina_stringshare_add(MANGAREADER_URL);
   cs->provider.search_index = MANGAREADER_SERIES_INDEX;
   cs->provider.index_start[0] = MANGAREADER_SERIES_INDEX_START;
   cs->provider.index_char[0] = MANGAREADER_SERIES_INDEX_START_CHAR;
   cs->provider.index_start[1] = MANGAREADER_SERIES_INDEX_POST_IMAGE;
   cs->provider.index_char[1] = MANGAREADER_SERIES_INDEX_POST_IMAGE_CHAR;
   cs->provider.index_start[2] = MANGAREADER_SERIES_INDEX_ALT_NAME;
   cs->provider.index_char[2] = MANGAREADER_SERIES_INDEX_ALT_NAME_CHAR;
   cs->provider.index_start[3] = MANGAREADER_SERIES_INDEX_YEAR;
   cs->provider.index_char[3] = MANGAREADER_SERIES_INDEX_YEAR_CHAR;
   cs->provider.index_start[4] = MANGAREADER_SERIES_INDEX_COMPLETED;
   cs->provider.index_char[4] = MANGAREADER_SERIES_INDEX_COMPLETED_CHAR;
   cs->provider.index_start[5] = MANGAREADER_SERIES_INDEX_AUTHOR;
   cs->provider.index_char[5] = MANGAREADER_SERIES_INDEX_AUTHOR_CHAR;
   cs->provider.index_start[6] = MANGAREADER_SERIES_INDEX_ARTIST;
   cs->provider.index_char[6] = MANGAREADER_SERIES_INDEX_ARTIST_CHAR;
   cs->provider.index_start[7] = MANGAREADER_SERIES_INDEX_JUMP;
   cs->provider.index_char[7] = MANGAREADER_SERIES_INDEX_JUMP_CHAR;
   cs->provider.data_cb = (Provider_Data_Cb)mangareader_comic_series_data_cb;
   cs->provider.init_cb = (Provider_Init_Cb)mangareader_comic_series_init_cb;
}

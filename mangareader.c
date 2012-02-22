#include "mangareader.h"

void
mangareader_search_name_cb(Search_Name *sn)
{
   const char *data;
   size_t size;

   if (sn->done) return;
   data = eina_strbuf_string_get(sn->buf);
   size = eina_strbuf_length_get(sn->buf);
   if ((!sn->idx[0]) && (!sn->idx[1]))
     sn->idx[0] = sn->provider.search_index + (sn->provider.search_name_count * sn->namelen);
   DBG("(idx=%u,size=%d)", sn->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (sn->idx[1] < sizeof(sn->provider.index_start)) && sn->provider.index_start[sn->idx[1]]; sn->idx[1]++)
     {
        Search_Result *sr = NULL;
        const char *p, *index_start;

        DBG("(idx=%u,size=%d)", sn->idx[0], size);
        if (sn->idx[0] + sn->provider.index_start[sn->idx[1]] + 8 > (unsigned int)size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             search_view_count_update(sn);
             return;
          }
        sn->idx[0] += sn->provider.index_start[sn->idx[1]];
        index_start = data + sn->idx[0];
        if (!memcmp(index_start, "adfooter", 8))
          {
             sn->done = EINA_TRUE;
             eina_strbuf_free(sn->buf);
             sn->buf = NULL;
             search_view_count_update(sn);
             return;
          }
        if (sn->provider.index_char[sn->idx[1]])
          p = memchr(index_start, sn->provider.index_char[sn->idx[1]], size - sn->idx[0]);
        switch (sn->idx[1])
          {
           case 0: /* result image (thumb) */
             sr = search_result_add(sn);
             sr->image.imgurl = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("imgurl=%s", sr->image.imgurl);
             {
                sr->image.ecu = ecore_con_url_new(sr->image.imgurl);
                ecore_con_url_data_set(sr->image.ecu, &sr->image);
                ecore_con_url_get(sr->image.ecu);
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
                char *tag;
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
        sn->idx[0] = p - data;
     }
}


static void
mangareader_comic_series_data_cb2(Comic_Series *cs)
{
   const char *base, *buf, *p;
   size_t size;

   if (!cs->done) return;
   base = buf = eina_strbuf_string_get(cs->buf);
   size = eina_strbuf_length_get(cs->buf);
   buf += cs->idx[0];
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
   eina_strbuf_free(cs->buf);
   cs->buf = NULL;
}

void
mangareader_comic_series_data_cb(Comic_Series *cs)
{
   const char *data;
   size_t size;

   if (cs->done)
     {
        mangareader_comic_series_data_cb2(cs);
        return;
     }

   data = eina_strbuf_string_get(cs->buf);
   size = eina_strbuf_length_get(cs->buf);
   if ((!cs->idx[0]) && (!cs->idx[1]))
     cs->idx[0] = cs->provider.search_index + (MANGAREADER_SERIES_INDEX_NAME_COUNT * cs->namelen);
   DBG("(idx=%u,size=%d)", cs->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (cs->idx[1] < sizeof(cs->provider.index_start)) && (cs->provider.index_start[cs->idx[1]] || cs->provider.index_char[cs->idx[1]]); cs->idx[1]++)
     {
        const char *p, *index_start;
        unsigned int jump = 0;

        DBG("(idx=%u,size=%d)", cs->idx[0], size);
        if (cs->idx[1] == 1) jump = MANGAREADER_SERIES_INDEX_POST_IMAGE_NAME_COUNT * cs->namelen;
        if (cs->idx[0] + cs->provider.index_start[cs->idx[1]] + jump > (unsigned int)size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             return;
          }
        cs->idx[0] += cs->provider.index_start[cs->idx[1]] + jump;
        index_start = data + cs->idx[0];
        if (cs->provider.index_char[cs->idx[1]])
          p = memchr(index_start, cs->provider.index_char[cs->idx[1]], size - cs->idx[0]);
        switch (cs->idx[1])
          {
           case 0: /* series image */
             cs->image.imgurl = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("imgurl=%s", cs->image.imgurl);
             {
                cs->image.ecu = ecore_con_url_new(cs->image.imgurl);
                ecore_con_url_data_set(cs->image.ecu, &cs->image);
                ecore_con_url_get(cs->image.ecu);
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
             break;
           default:
             cs->idx[1]++;
             return;
          }
        cs->idx[0] = p - data;
     }
}

void
mangareader_comic_page_data_cb(Comic_Page *cp)
{
   const char *data = eina_strbuf_string_get(cp->buf);
   size_t size = eina_strbuf_length_get(cp->buf);

   if ((!cp->idx[0]) && (!cp->idx[1]))
     cp->idx[0] = cp->provider.search_index + (MANGAREADER_PAGE_INDEX_NAME_COUNT * cp->cc->cs->namelen);
   if (cp->number > 9)
     cp->idx[0] += 3;
   if (cp->number > 99)
     cp->idx[0] += 3;
   if (cp->number > 999)
     cp->idx[0] += 3;
   DBG("(idx=%u,size=%d)", cp->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (cp->idx[1] < sizeof(cp->provider.index_start)) && (cp->provider.index_start[cp->idx[1]] || cp->provider.index_char[cp->idx[1]]); cp->idx[1]++)
     {
        const char *p, *index_start;

        DBG("(idx=%u,size=%d)", cp->idx[0], size);
        if (cp->idx[0] + cp->provider.index_start[cp->idx[1]] > size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             return;
          }
        cp->idx[0] += cp->provider.index_start[cp->idx[1]];
        index_start = data + cp->idx[0];
        if (cp->provider.index_char[cp->idx[1]])
          p = memchr(index_start, cp->provider.index_char[cp->idx[1]], size - cp->idx[0]);
        switch (cp->idx[1])
          {
           case 0: /* placeholder */
           case 1: /* placeholder */
             break;
           case 2: /* next page href */
             {
                Comic_Page *cn;
                const char *dash;
                unsigned int num;

                dash = (char*)memchr(p - 12, '-', 12);
                num = strtoul(dash + 1, NULL, 10);
                if (num == cp->cc->number)
                  cn = comic_page_new(cp->cc, cp->number + 1);
                else
                  {
                     if (!EINA_INLIST_GET(cp->cc)->next)
                       {
                          /* this should be impossible if I'm not a moron */
                          CRI("PREPARING TO CRASH");
                          abort();
                       }
                     cn = comic_page_new(EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(cp->cc)->next, Comic_Chapter), 1);
                  }
                cn->href = eina_stringshare_add_length(index_start, p - index_start);
             }
             break;
           case 3: /* placeholder */
             break;
           case 4: /* actual page image */
             if (p - index_start > 1)
               cp->image.imgurl = eina_stringshare_add_length(index_start, p - index_start);
             else
               {
                  /* FIXME: this is super slow by comparison */
                  const char *pp;

                  pp = strstr(data + (size - 4500), "imgholder");
                  pp = strstr(pp + 72, "src=");
                  pp += 5;
                  p = strchr(pp, '"');
                  cp->image.imgurl = eina_stringshare_add_length(pp, p - pp);
               }
             INF("imgurl=%s", cp->image.imgurl);
             {
                cp->ecu = ecore_con_url_new(cp->image.imgurl);
                ecore_con_url_data_set(cp->ecu, &cp->image);
                ecore_con_url_get(cp->ecu);
             }
             break;
           default:
             cp->done = EINA_TRUE;
             return;
          }
        cp->idx[0] = p - data;
     }
}

void
mangareader_comic_page_init_cb(Comic_Page *cp)
{
   cp->provider.url = eina_stringshare_add(MANGAREADER_URL);
   cp->provider.search_index = MANGAREADER_PAGE_INDEX;
   cp->provider.index_start[0] = MANGAREADER_PAGE_INDEX_START;
   cp->provider.index_char[0] = MANGAREADER_PAGE_INDEX_START_CHAR;
   cp->provider.index_start[1] = MANGAREADER_PAGE_INDEX_JUMP;
   cp->provider.index_char[1] = MANGAREADER_PAGE_INDEX_JUMP_CHAR;
   cp->provider.index_start[2] = MANGAREADER_PAGE_INDEX_NEXT;
   cp->provider.index_char[2] = MANGAREADER_PAGE_INDEX_NEXT_CHAR;
   cp->provider.index_start[3] = MANGAREADER_PAGE_INDEX_PREV;
   cp->provider.index_char[3] = MANGAREADER_PAGE_INDEX_PREV_CHAR;
   cp->provider.index_start[4] = MANGAREADER_PAGE_INDEX_IMG;
   cp->provider.index_char[4] = MANGAREADER_PAGE_INDEX_IMG_CHAR;
   cp->provider.data_cb = (Provider_Data_Cb)mangareader_comic_page_data_cb;
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
   cs->provider.init_cb = (Provider_Init_Cb)mangareader_comic_page_init_cb;
}

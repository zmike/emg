#include "mangareader.h"

static void mangareader_search_name_cb(Search_Name *sn);
static void mangareader_comic_series_data_cb2(Comic_Series *cs);
static void mangareader_comic_series_data_cb(Comic_Series *cs);
static void mangareader_comic_page_data_cb(Comic_Page *cp);
static void mangareader_comic_page_init_cb(Comic_Page *cp);
static void mangareader_series_init_cb(Comic_Series *cs);

static Comic_Provider search_provider =
{
   .url = MANGAREADER_URL,
   .priority = MANGAREADER_PROVIDER_PRIORITY,
   .search_url = MANGAREADER_SEARCH_URL,
   .search_index = MANGAREADER_SEARCH_INDEX,
   .search_name_count = MANGAREADER_SEARCH_INDEX_NAME_COUNT,
   .index_start[0] = MANGAREADER_SEARCH_INDEX_START,
   .index_char[0] = MANGAREADER_SEARCH_INDEX_START_CHAR,
   .index_start[1] = MANGAREADER_SEARCH_INDEX_POST_IMAGE,
   .index_char[1] = MANGAREADER_SEARCH_INDEX_POST_IMAGE_CHAR,
   .index_start[2] = MANGAREADER_SEARCH_INDEX_POST_HREF,
   .index_char[2] = MANGAREADER_SEARCH_INDEX_POST_HREF_CHAR,
   .index_start[3] = MANGAREADER_SEARCH_INDEX_CHAP,
   .index_char[3] = MANGAREADER_SEARCH_INDEX_CHAP_CHAR,
   .index_start[4] = MANGAREADER_SEARCH_INDEX_TAGS,
   .index_char[4] = MANGAREADER_SEARCH_INDEX_TAGS_CHAR,
   .index_start[5] = MANGAREADER_SEARCH_INDEX_END,
   .index_char[5] = MANGAREADER_SEARCH_INDEX_END_CHAR,
   .replace_str = MANGAREADER_REPLACE_STR,
   .data_cb = MANGAREADER_DATA_CB,
   .init_cb = MANGAREADER_INIT_CB
};

static Comic_Provider series_provider =
{
   .url = MANGAREADER_URL,
   .priority = MANGAREADER_PROVIDER_PRIORITY,
   .search_index = MANGAREADER_SERIES_INDEX,
   .index_start[0] = MANGAREADER_SERIES_INDEX_START,
   .index_char[0] = MANGAREADER_SERIES_INDEX_START_CHAR,
   .index_start[1] = MANGAREADER_SERIES_INDEX_POST_IMAGE,
   .index_char[1] = MANGAREADER_SERIES_INDEX_POST_IMAGE_CHAR,
   .index_start[2] = MANGAREADER_SERIES_INDEX_ALT_NAME,
   .index_char[2] = MANGAREADER_SERIES_INDEX_ALT_NAME_CHAR,
   .index_start[3] = MANGAREADER_SERIES_INDEX_YEAR,
   .index_char[3] = MANGAREADER_SERIES_INDEX_YEAR_CHAR,
   .index_start[4] = MANGAREADER_SERIES_INDEX_COMPLETED,
   .index_char[4] = MANGAREADER_SERIES_INDEX_COMPLETED_CHAR,
   .index_start[5] = MANGAREADER_SERIES_INDEX_AUTHOR,
   .index_char[5] = MANGAREADER_SERIES_INDEX_AUTHOR_CHAR,
   .index_start[6] = MANGAREADER_SERIES_INDEX_ARTIST,
   .index_char[6] = MANGAREADER_SERIES_INDEX_ARTIST_CHAR,
   .index_start[7] = MANGAREADER_SERIES_INDEX_JUMP,
   .index_char[7] = MANGAREADER_SERIES_INDEX_JUMP_CHAR,
   .data_cb = (Provider_Data_Cb)mangareader_comic_series_data_cb,
   .init_cb = (Provider_Init_Cb)mangareader_comic_page_init_cb
};

static Comic_Provider page_provider =
{
   .url = MANGAREADER_URL,
   .priority = MANGAREADER_PROVIDER_PRIORITY,
   .search_index = MANGAREADER_PAGE_INDEX,
   .index_start[0] = MANGAREADER_PAGE_INDEX_START,
   .index_char[0] = MANGAREADER_PAGE_INDEX_START_CHAR,
   .index_start[1] = MANGAREADER_PAGE_INDEX_JUMP,
   .index_char[1] = MANGAREADER_PAGE_INDEX_JUMP_CHAR,
   .index_start[2] = MANGAREADER_PAGE_INDEX_NEXT,
   .index_char[2] = MANGAREADER_PAGE_INDEX_NEXT_CHAR,
   .index_start[3] = MANGAREADER_PAGE_INDEX_PREV,
   .index_char[3] = MANGAREADER_PAGE_INDEX_PREV_CHAR,
   .index_start[4] = MANGAREADER_PAGE_INDEX_IMG,
   .index_char[4] = MANGAREADER_PAGE_INDEX_IMG_CHAR,
   .data_cb = (Provider_Data_Cb)mangareader_comic_page_data_cb
};

static void
mangareader_search_name_cb(Search_Name *sn)
{
   const char *data;
   size_t size;

   if (sn->done) return;
   data = eina_strbuf_string_get(sn->buf);
   size = eina_strbuf_length_get(sn->buf);
   if ((!sn->idx[0]) && (!sn->idx[1]))
     sn->idx[0] = sn->provider->search_index + (sn->provider->search_name_count * sn->namelen);
   //DBG("(idx=%u,size=%d)", sn->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (sn->idx[1] < sizeof(sn->provider->index_start)) && sn->provider->index_start[sn->idx[1]]; sn->idx[1]++)
     {
        Search_Result *sr = NULL;
        const char *p, *index_start;

        //DBG("(idx=%u,size=%d)", sn->idx[0], size);
        if (sn->idx[0] + sn->provider->index_start[sn->idx[1]] + 8 > (unsigned int)size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             search_view_count_update(sn);
             return;
          }
        index_start = data + sn->idx[0] + sn->provider->index_start[sn->idx[1]];
        if (!memcmp(index_start, "adfooter", 8))
          {
             sn->done = EINA_TRUE;
             eina_strbuf_free(sn->buf);
             sn->buf = NULL;
             search_view_count_update(sn);
             return;
          }
        if (sn->provider->index_char[sn->idx[1]])
          {
             p = memchr(index_start, sn->provider->index_char[sn->idx[1]], size - sn->idx[0]);
             if (!p) return;
          }
        switch (sn->idx[1])
          {
           case 0: /* result image (thumb) */
             sr = search_result_add(sn);
             sr->image.href = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("href=%s", sr->image.href);
             {
                sr->image.ecu = ecore_con_url_new(sr->image.href);
                ecore_con_url_data_set(sr->image.ecu, &sr->image);
                ecore_con_url_get(sr->image.ecu);
             }
             break;
           case 1: /* result link */
             sr = EINA_INLIST_CONTAINER_GET(sn->results->last, Search_Result);
             {
                char *buf;
                buf = strndupa(index_start, p - index_start);
                sr->href = eina_stringshare_printf("%s%s", sn->provider->url, buf);
             }
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
             p = index_start;
          }
        sn->idx[0] = p - data;
     }
}


static void
mangareader_comic_series_data_cb2(Comic_Series *cs)
{
   const char *base, *buf, *p;
   size_t size;

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
        cc = comic_chapter_new(cs, EINA_FALSE);
        pp = strndupa(buf, p - buf);
        cc->href = eina_stringshare_printf("%s%s", cs->provider->url, pp);
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
}

static void
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
     cs->idx[0] = cs->provider->search_index + (MANGAREADER_SERIES_INDEX_NAME_COUNT * cs->namelen);
   //DBG("(idx=%u,size=%d)", cs->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (cs->idx[1] < sizeof(cs->provider->index_start)) && (cs->provider->index_start[cs->idx[1]] || cs->provider->index_char[cs->idx[1]]); cs->idx[1]++)
     {
        const char *p, *index_start;
        unsigned int jump = 0;

        //DBG("(idx=%u,size=%d)", cs->idx[0], size);
        if (cs->idx[1] == 1) jump = MANGAREADER_SERIES_INDEX_POST_IMAGE_NAME_COUNT * cs->namelen;
        if (cs->idx[0] + cs->provider->index_start[cs->idx[1]] + jump > (unsigned int)size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             return;
          }
        index_start = data + cs->idx[0] + cs->provider->index_start[cs->idx[1]] + jump;
        if (cs->provider->index_char[cs->idx[1]])
          {
             p = memchr(index_start, cs->provider->index_char[cs->idx[1]], size - cs->idx[0]);
             if (!p) return;
          }
        switch (cs->idx[1])
          {
           case 0: /* series image */
             cs->image.href = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("href=%s", cs->image.href);
             {
                cs->image.ecu = ecore_con_url_new(cs->image.href);
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

static void
mangareader_comic_page_data_cb(Comic_Page *cp)
{
   const char *data;
   size_t size;

   if (cp->done || (!cp->buf)) return;
   data = eina_strbuf_string_get(cp->buf);
   size = eina_strbuf_length_get(cp->buf);
   if ((!cp->idx[0]) && (!cp->idx[1]))
     {
          cp->idx[0] = cp->provider->search_index + (MANGAREADER_PAGE_INDEX_NAME_COUNT * cp->cc->cs->namelen);
        if (cp->cc->number > 9)
          cp->idx[0] += 8;
        if (cp->cc->number > 99)
          cp->idx[0] += 8;
        if (cp->cc->number > 999)
          cp->idx[0] += 8;
        if (cp->number > 9)
          cp->idx[0] += 3;
        if (cp->number > 99)
          cp->idx[0] += 3;
        if (cp->number > 999)
          cp->idx[0] += 3;
     }
   //DBG("(idx=%u,size=%d)", cp->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (cp->idx[1] < sizeof(cp->provider->index_start)) && (cp->provider->index_start[cp->idx[1]] || cp->provider->index_char[cp->idx[1]]); cp->idx[1]++)
     {
        const char *p, *index_start;

        //DBG("(idx=%u,size=%d)", cp->idx[0], size);
        if (cp->idx[0] + cp->provider->index_start[cp->idx[1]] > size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             return;
          }
        index_start = data + cp->idx[0] + cp->provider->index_start[cp->idx[1]];
        if ((!cp->idx[1]) && (!memcmp(index_start, "ader", 4)))
          /* shouldn't get here, fix this if it happens */
          abort();
        if (cp->provider->index_char[cp->idx[1]])
          {
             p = memchr(index_start, cp->provider->index_char[cp->idx[1]], size - cp->idx[0]);
             if (!p) return;
          }
        switch (cp->idx[1])
          {
           case 0: /* placeholder */
           case 1: /* placeholder */
             break;
           case 2: /* next page href */
           case 3:
             {
                Comic_Page *cn;
                const char *pp, *ppp;
                unsigned int cnum = 0, pnum = 0;

                if (index_start == p) break; /* no uri */
                if ((cp->idx[1] == 2) && comic_page_next_get(cp))
                  /* already have a next page, no need to do anything here */
                  break;
                if ((cp->idx[1] == 3) && comic_page_prev_get(cp))
                  /* already have a previous page, no need to do anything here */
                  break;
                pp = index_start + 1;
                pp = strchr(pp, '/');
                if (!pp) abort(); /* need to implement parser here */
                if (isdigit(p[-1]))
                  {
                                 /* V */
                     /* /SERIES_NAME/CHAPTER_NUMBER/(PAGE_NUMBER)? */
                     cnum = strtoul(pp + 1, (char**)&ppp, 10);
                     if (isdigit(ppp[1]))
                       pnum = strtoul(ppp + 1, NULL, 10);
                  }
                else
                  {
                                               /* V */
                     /* /NUMBER-NUMBER-PAGE_NUMBER/SERIES_NAME/chapter-CHAPTER_NUMBER.html */
                     pp = memrchr(index_start, '-', (pp - 1) - index_start);
                     pnum = strtoul(pp + 1, (char**)&ppp, 10);
                     pp = (char*)memchr(p - 12, '-', 12);
                     cnum = strtoul(pp + 1, NULL, 10);
                  }
                if (cnum == cp->cc->number)
                  cn = comic_page_new(cp->cc, pnum ?: 1);
                else
                  {
                     Comic_Chapter *cc;

                     if (cp->idx[1] == 2)
                       cc = comic_chapter_next_get(cp->cc);
                     else
                       cc = comic_chapter_prev_get(cp->cc);
                     if (!cc)
                       {
                          /* no more chapters available in this direction, shouldn't get here */
                          break;
                       }
                     cn = comic_page_new(cc, pnum);
                  }
                {
                   char *buf;
                   buf = strndupa(index_start, p - index_start);
                   cn->href = eina_stringshare_printf("%s%s", cn->provider->url, buf);
                }
             }
             break;
           case 4: /* actual page image */
             if (p - index_start > 1)
               cp->image.href = eina_stringshare_add_length(index_start, p - index_start);
             else
               {
                  /* FIXME: this is super slow by comparison */
                  const char *pp;

                  if (size <= 5100) return;
                  pp = strstr(data + (size - 5000), "imgholder");
                  if (!pp) return; /* need more data */
                  if (size < (unsigned int)((pp - data) + 60)) return;
                  pp = strstr(pp + 60, "src=");
                  if (size < (unsigned int)((pp - data) + 5)) return;
                  pp += 5;
                  p = strchr(pp, '"');
                  cp->image.href = eina_stringshare_add_length(pp, p - pp);
               }
             INF("%u href=%s", cp->number, cp->image.href);
             break;
           default:
             cp->done = EINA_TRUE;
             return;
          }
        cp->idx[0] = p - data;
     }
}

static void
mangareader_comic_page_init_cb(Comic_Page *cp)
{
   cp->provider = &page_provider;
}

static void
mangareader_series_init_cb(Comic_Series *cs)
{
   cs->provider = &series_provider;
}

void
mangareader_search_init_cb(Search_Name *sn)
{
   sn->provider = &search_provider;
}

#include "batoto.h"

static void
batoto_search_name_cb(Search_Name *sn)
{
   const char *data;
   size_t size;

   if (sn->done) return;
   data = eina_strbuf_string_get(sn->buf);
   size = eina_strbuf_length_get(sn->buf);
   if ((!sn->idx[0]) && (!sn->idx[1]))
     {
        char *s;
        sn->idx[0] = sn->provider.search_index + (2 * sn->snamelen) + sn->namelen;
        for (s = strchr(sn->name, '\''); s; s = strchr(s + 1, '\''))
          sn->idx[0] += (5 * 2);
     }
   for (; (sn->idx[1] < sizeof(sn->provider.index_start)) && sn->provider.index_start[sn->idx[1]]; sn->idx[1]++)
     {
        Search_Result *sr = NULL;
        const char *p, *index_start;

        if (sn->idx[0] + sn->provider.index_start[sn->idx[1]] + 8 > (unsigned int)size)
          {
             search_view_count_update(sn);
             return;
          }
        index_start = data + sn->idx[0] + sn->provider.index_start[sn->idx[1]];
        if ((!sn->idx[1]) && memcmp(index_start, "http", 4))
          {
             sn->done = EINA_TRUE;
             search_view_count_update(sn);
             return;
          }
        if (sn->provider.index_char[sn->idx[1]])
          {
             p = memchr(index_start, sn->provider.index_char[sn->idx[1]], size - sn->idx[0]);
             if (!p) return;
          }
        switch (sn->idx[1])
          {
           case 0: /* result link */
             sr = search_result_add(sn);
             sr->href = eina_stringshare_add_length(index_start, p - index_start);
             INF("href=%s", sr->href);
             break;
           case 1: /* result name */
             sr = EINA_INLIST_CONTAINER_GET(sn->results->last, Search_Result);
             {
                char *buf;

                buf = strndupa(index_start, p - index_start);
                buf = evas_textblock_text_markup_to_utf8(NULL, buf);
                sr->name = eina_stringshare_add(buf);
                free(buf);
             }
             sr->namelen = p - index_start;
             INF("name=%s", sr->name);
             sr->it = elm_genlist_item_append(sn->e->sw.list, &sn->e->sw.itc, sr, NULL, 0, (Evas_Smart_Cb)NULL, sr);
             break;
           case 6:
             sn->idx[1] = -1;
             p = index_start;
             break;
           default:
             break;
          }
        sn->idx[0] = p - data;
     }
}
#define BUFCHECK(NUM) \
        if ((unsigned int)((p + NUM) - base) >= size) abort(); /* FIXME */ \
        buf = p + NUM

#define BUFCHR(CHAR) \
        p = strchr(buf, CHAR); \
        if (!p) abort() /* FIXME */


static void
batoto_comic_series_data_cb2(Comic_Series *cs)
{
   const char *base, *buf, *p;
   size_t size;
   int chapters = 0;
   Eina_Bool use_ch = EINA_FALSE;
   Comic_Chapter *cc = NULL;

   base = buf = eina_strbuf_string_get(cs->buf);
   size = eina_strbuf_length_get(cs->buf);
   buf += cs->idx[0];
   chapters = (size - BATOTO_SERIES_INDEX_TRAILING - BATOTO_SERIES_INDEX_JUMP) / BATOTO_SERIES_INDEX_ROW_SIZE;
   p = strstr(buf, "<tr");
   if (!p) abort();
   BUFCHECK(11);
   if (buf[0] == 'r') buf -= 11;
   while (1)
     {
        Comic_Page *cp;
        Comic_Chapter *ccp;
        const char *href, *hrefend;
        Eina_Bool update = EINA_FALSE, decimal = EINA_FALSE;
        double number = 0;

        p = strstr(buf, "<tr");
        if (!p) abort(); /* FIXME */
        BUFCHECK(11);
        if (!memcmp(buf, "chap_avl", 8))
          break; /* DONE! */
        BUFCHECK(20);
        if (memcmp(buf, "English", 7))
          {
             p = buf;
             BUFCHECK(BATOTO_SERIES_INDEX_ROW_SIZE - 20);
             continue;
          }
        BUFCHECK(85);
        if (buf[0] != 'h') abort();
        BUFCHR('"');
        href = buf, hrefend = p;
        BUFCHECK(3);
        BUFCHR('>');
        BUFCHECK(2);
        if ((buf[0] == 'V') || (buf[0] == 'v'))
          {
             /* Vol.X Ch.Y */
             BUFCHR(' ');
             BUFCHECK(1);
          }
        /* Ch.Y */
        p = buf;
        if (memcmp(buf, "Ch.", 3))
          decimal = EINA_TRUE;
        else
          {
             BUFCHECK(3);
             if (isdigit(buf[0]))
               {
                  number = strtod(buf, (char**)&p);
                  if (p - buf > 2)
                    {
                       buf = memchr(buf, '.', p - buf);
                       if (buf) decimal = EINA_TRUE;
                    }
               }
          }
        if (cs->total == 1)
          {
             if ((abs((int)number - chapters) < 10) || (number > 12))
               {
                  DBG("chapter numbers appear valid");
                  use_ch = EINA_TRUE;
               }
             else
               number = chapters; /* adjust later */
          }
        if (use_ch && cs->chapters)
          {
             ccp = EINA_INLIST_CONTAINER_GET(cs->chapters, Comic_Chapter);
             if (ccp->number == number)
               {
                  update = EINA_TRUE;
                  cc = ccp;
               }
          }
        if ((!decimal) && (!update)) cs->total++;
        if (!update)
          {
             cc = comic_chapter_new(cs, EINA_TRUE);
             if (use_ch)
               cc->number = number;
          }
        if (!cc->href)
          cc->href = eina_stringshare_add_length(href, hrefend - href);
        if (!update)
          {
             cp = comic_page_new(cc, 1);
             cp->href = eina_stringshare_ref(cc->href);
          }
        else if (!use_ch)
          {
             Comic_Chapter *ccn;

             ccn = comic_chapter_next_get(cc);
             cc->number = ccn->decimal ? ((int)ccn->number) : (int)ccn->number - 1;
             if (cc->decimal) cc->number += 0.5;
          }
        buf = p;
        BUFCHR(' ');
        BUFCHECK(1);
        BUFCHR('<');
        if ((p - buf > 1) && ((p - buf != 11) || memcmp(buf, "Read Online", 11)))
          {
             if (!memcmp(buf, "Ch.", 3))
               {
                  buf += 3;
                  cc->number--;
                  cs->total--;
                  cc->decimal = EINA_TRUE;
               }
             if (!cc->name)
               {
                  char *buf;

                  buf = strndupa(buf, p - buf);
                  buf = evas_textblock_text_markup_to_utf8(NULL, buf);
                  cc->name = eina_stringshare_add(buf);
                  free(buf);
               }
          }
        INF("chapter: %g - %s: %s", cc->number, cc->name, cc->href);
     }
   if (!use_ch)
     {
        int adjust2 = 0;
        chapters =  chapters - cs->total;
        INF("ADJUST: %d", chapters);
        for (; cc; cc = comic_chapter_next_get(cc))
          {
             char buf[32];
             cc->number -= (chapters + adjust2);
             if (!cc->decimal) continue;
             snprintf(buf, sizeof(buf), "%g", cc->number);
             if (!strchr(buf, '.')) cc->number += 0.5;
             adjust2++;
          }
     }
}

static void
batoto_comic_series_data_cb(Comic_Series *cs)
{
   const char *data;
   size_t size;

   if (cs->done && (cs->idx[1] == 8))
     {
        batoto_comic_series_data_cb2(cs);
        return;
     }

   data = eina_strbuf_string_get(cs->buf);
   size = eina_strbuf_length_get(cs->buf);
   if ((!cs->idx[0]) && (!cs->idx[1]))
     cs->idx[0] = cs->provider.search_index;
   //DBG("(idx=%u,size=%d)", cs->idx[0], size);
   /* discard unneeded bytes, hooray */
   for (; (cs->idx[1] < sizeof(cs->provider.index_start)) && (cs->provider.index_start[cs->idx[1]] || cs->provider.index_char[cs->idx[1]]); cs->idx[1]++)
     {
        const char *p, *index_start;

        //DBG("(idx=%u,size=%d)", cs->idx[0], size);
        if (cs->idx[0] + cs->provider.index_start[cs->idx[1]] > (unsigned int)size)
          {
             /*
             char *buf;
             buf = strndupa((char*)data, size);
             DBG("%s", buf);
             */
             return;
          }
        index_start = data + cs->idx[0] + cs->provider.index_start[cs->idx[1]];
        if (cs->provider.index_char[cs->idx[1]])
          {
             p = memchr(index_start, cs->provider.index_char[cs->idx[1]], size - cs->idx[0]);
             if (!p) return;
          }
        switch (cs->idx[1])
          {
           case 0: /* series image */
             p = strstr(index_start, BATOTO_SERIES_INDEX_START_STR);
             if (!p)
               {
                  cs->idx[0] = size;
                  return;
               }
             break;
           case 1:
             cs->image.href = eina_stringshare_add_length((char*)index_start, p - index_start);
             INF("href=%s", cs->image.href);
             {
                cs->image.ecu = ecore_con_url_new(cs->image.href);
                ecore_con_url_data_set(cs->image.ecu, &cs->image);
                ecore_con_url_get(cs->image.ecu);
             }
             break;
           case 2: /* alt name */
             if ((!cs->alt_name) && (p - index_start > 0))
               {
                  cs->alt_name = eina_stringshare_add_length((char*)index_start, p - index_start);
                  INF("alt_name=%s", cs->alt_name);
                  series_view_title_set(cs->e, cs);
               }
             if ((unsigned int)(p - data) + 200 > size) return;
             p = strchr(p, '\r');
             break;
           case 3: /* author */
             if (p - index_start > 0)
               {
                  cs->author = eina_stringshare_add_length((char*)index_start, p - index_start);
                  INF("author=%s", cs->author);
                  series_view_author_set(cs->e, cs);
                  p += strlen(cs->author);
               }
             break;
           case 4: /* artist */
             if (p - index_start > 0)
               {
                  cs->artist = eina_stringshare_add_length((char*)index_start, p - index_start);
                  INF("artist=%s", cs->artist);
                  series_view_artist_set(cs->e, cs);
                  p += strlen(cs->artist);
               }
             break;
           case 5:
             break;
           case 6:
             if ((unsigned int)((p + 10) - data) > size) return;
             index_start = p + 1;
             p = strchr(index_start, '<');
             if (!p) return;
             cs->completed = (p - index_start != 7);
             if ((p - index_start != 7) && (p - index_start != 9)) abort();
             break;
           case 7:
             cs->desc = strndup(index_start, p - index_start);
             series_view_desc_set(cs->e, cs);
             INF("desc=%s", cs->desc);
             cs->idx[1]++;
             cs->idx[0] = BATOTO_SERIES_INDEX_JUMP;
             if (!cs->done) return;
             batoto_comic_series_data_cb2(cs);
           default:
             return;
          }
        cs->idx[0] = p - data;
     }
}

static void
batoto_comic_page_data_cb(Comic_Page *cp)
{
   const char *p, *index_start;
   const char *data;
   size_t size;

   if (!cp->buf) return;
   data = eina_strbuf_string_get(cp->buf);
   size = eina_strbuf_length_get(cp->buf);
   if ((!cp->idx[0]) && (!cp->idx[1]))
     cp->idx[0] = cp->provider.search_index + (cp->cc->cs->total * BATOTO_PAGE_INDEX_CHAPTER_JUMP);
   if (cp->idx[0] >= size) return;

   index_start = data + cp->idx[0];
   switch (cp->idx[1])
     {
      case 0:
        for (; cp->idx[1] < 2; cp->idx[1]++)
          {
             p = strstr(index_start, "<se");
             if (!p)
               {
                  cp->idx[0] = size;
                  return;
               }
             cp->idx[0] = p - data;
             index_start = data + cp->idx[0];
          }
        index_start = p;
        cp->idx[1] = 1;
      case 1:
        p = strstr(index_start, "</li");
        if (!p)
          {
             cp->idx[0] = size;
             return;
          }
        cp->idx[0] = p - data;
        cp->idx[1] = 2;
        index_start = p;
      case 2:
        if (cp->idx[0] + 6300 >= size) return;
        index_start = strchr(index_start + 1, '<');
        if (!index_start) abort();
        if (index_start[1] != 's')
          {
             cp->idx[1] = 5;
             batoto_comic_page_data_cb(cp);
             return;
          }
        index_start += 90;
        if (index_start[0] != '<') abort(); /* FIXME */
        cp->idx[0] = index_start - data;
        cp->idx[1] = 3;
      case 3:
        if (cp->cc->page_count == 1)
          {
             while (1)
               {
                  const char *pp;
                  unsigned int num = 0;
                  Comic_Page *cpn;
                  p = strchr(index_start + 15, '"');
                  if (!p) return;
                  index_start += 15;
                  pp = p - 1;
                  if (isdigit(pp[0]))
                    {
                       while (isdigit(pp[-1])) pp--;
                       num = strtoul(pp, NULL, 10);
                    }
                  if (p[2] == 's')
                    /* current page */
                    {
                       if (cp->number && num && (num != cp->number))
                         {
                            DBG("PAGE NUMBER FIX: %u to %u", cp->number, num);
                            cp->number = num;
                         }
                       index_start = p + 21;
                    }
                  else
                    {
                       cpn = comic_page_new(cp->cc, num);
                       cpn->href = eina_stringshare_add_length(index_start, p - index_start);
                       index_start = p + 2;
                    }
                  p = strchr(index_start + 1, '>');
                  if (!p) abort(); /* FIXME */
                  if (p[1] != '<')
                    {
                       cp->idx[1] = 4;
                       cp->idx[0] = p - data;
                       batoto_comic_page_data_cb(cp);
                       return;
                    }
                  index_start = p + 1;
                  cp->idx[0] = index_start - data;
               }
          }
        cp->idx[1] = 4;
      case 4:
        do
          {
             if (!comic_page_prev_get(cp))
               {
                  Comic_Chapter *ccp;
                  Comic_Page *cpp;

                  ccp = comic_chapter_prev_get(cp->cc);
                  if (!ccp) break; /* already have this url or no previous ch */
                  p = strstr(index_start + 200, "</li");
                  if (!p) abort(); /* FIXME */
                  index_start = p + 110;
                  p = strchr(index_start, '"');
                  if (!p) abort(); /* FIXME */
                  cpp = comic_page_new(ccp, 999);
                  cpp->href = eina_stringshare_add_length(index_start, p - index_start);
               }
          } while (0);
        p = strstr(index_start, "\"fu");
        if (!p)
          {
             cp->idx[0] = size;
             return;
          }
        index_start = p + 400;
        p = strchr(index_start, '"');
        if (!p)
          {
             cp->idx[0] = size;
             return;
          }
        cp->image.href = eina_stringshare_add_length(index_start, p - index_start);
        INF("%u href=%s", cp->number, cp->image.href);
        {
           cp->image.ecu = ecore_con_url_new(cp->image.href);
           ecore_con_url_data_set(cp->image.ecu, &cp->image);
           ecore_con_url_get(cp->image.ecu);
        }
        eina_strbuf_free(cp->buf);
        cp->buf = NULL;
        return;
      case 5:
        index_start += 1500, cp->idx[0] += 1500;
        p = strstr(index_start, "15p");
        if (!p) abort(); /* FIXME */
        index_start = p + 2;
        for (; index_start[5] == '<'; index_start = strchr(p, '<'))
          {
             index_start += 15;
             p = strchr(index_start, '\'');
             if (!p) abort(); /* FIXME */
             if (cp->image.href)
               {
                  Comic_Page *cpn;

                  cpn = comic_page_new(cp->cc, cp->cc->page_count + 1);
                  cpn->image.href = eina_stringshare_add_length(index_start, p - index_start);
               }
             else
               cp->image.href = eina_stringshare_add_length(index_start, p - index_start);
          }
        eina_strbuf_free(cp->buf);
        cp->buf = NULL;
      default:
        return;
     }
}

static void
batoto_comic_page_init_cb(Comic_Page *cp)
{
   cp->provider.url = eina_stringshare_add(BATOTO_URL);
   cp->provider.search_index = BATOTO_PAGE_INDEX;
   cp->provider.data_cb = (Provider_Data_Cb)batoto_comic_page_data_cb;
}

static void
batoto_series_init_cb(Comic_Series *cs)
{
   cs->provider.url = eina_stringshare_add(BATOTO_URL);
   cs->provider.search_index = BATOTO_SERIES_INDEX;
   cs->provider.index_start[0] = BATOTO_SERIES_INDEX_START;
   cs->provider.index_char[0] = BATOTO_SERIES_INDEX_START_CHAR;
   cs->provider.index_start[1] = BATOTO_SERIES_INDEX_IMAGE;
   cs->provider.index_char[1] = BATOTO_SERIES_INDEX_IMAGE_CHAR;
   cs->provider.index_start[2] = BATOTO_SERIES_INDEX_ALT_NAME;
   cs->provider.index_char[2] = BATOTO_SERIES_INDEX_ALT_NAME_CHAR;
   cs->provider.index_start[3] = BATOTO_SERIES_INDEX_AUTHOR;
   cs->provider.index_char[3] = BATOTO_SERIES_INDEX_AUTHOR_CHAR;
   cs->provider.index_start[4] = BATOTO_SERIES_INDEX_ARTIST;
   cs->provider.index_char[4] = BATOTO_SERIES_INDEX_ARTIST_CHAR;
   cs->provider.index_start[5] = BATOTO_SERIES_INDEX_AUTHOR;
   cs->provider.index_char[5] = BATOTO_SERIES_INDEX_AUTHOR_CHAR;
   cs->provider.index_start[6] = BATOTO_SERIES_INDEX_PRE_COMPLETED;
   cs->provider.index_char[6] = BATOTO_SERIES_INDEX_PRE_COMPLETED_CHAR;
   cs->provider.index_start[7] = BATOTO_SERIES_INDEX_DESC;
   cs->provider.index_char[7] = BATOTO_SERIES_INDEX_DESC_CHAR;
   cs->provider.data_cb = (Provider_Data_Cb)batoto_comic_series_data_cb;
   cs->provider.init_cb = (Provider_Init_Cb)batoto_comic_page_init_cb;
}

void
batoto_search_init_cb(Search_Name *sn)
{
   sn->provider.url = eina_stringshare_add(BATOTO_URL);
   sn->provider.search_url = eina_stringshare_add(BATOTO_SEARCH_URL);
   sn->provider.search_index = BATOTO_SEARCH_INDEX;
   sn->provider.index_start[0] = BATOTO_SEARCH_INDEX_START;
   sn->provider.index_char[0] = BATOTO_SEARCH_INDEX_START_CHAR;
   sn->provider.index_start[1] = BATOTO_SEARCH_INDEX_POST_HREF;
   sn->provider.index_char[1] = BATOTO_SEARCH_INDEX_POST_HREF_CHAR;
   sn->provider.index_start[2] = BATOTO_SEARCH_INDEX_POST_NAME;
   sn->provider.index_char[2] = BATOTO_SEARCH_INDEX_POST_NAME_CHAR;
   sn->provider.index_start[3] = BATOTO_SEARCH_INDEX_POST_AUTHOR;
   sn->provider.index_char[3] = BATOTO_SEARCH_INDEX_POST_AUTHOR_CHAR;
   sn->provider.index_start[4] = BATOTO_SEARCH_INDEX_POST_VIEWS;
   sn->provider.index_char[4] = BATOTO_SEARCH_INDEX_POST_VIEWS_CHAR;
   sn->provider.index_start[5] = BATOTO_SEARCH_INDEX_POST_FOLLOWS;
   sn->provider.index_char[5] = BATOTO_SEARCH_INDEX_POST_FOLLOWS_CHAR;
   sn->provider.index_start[6] = BATOTO_SEARCH_INDEX_END;
   sn->provider.index_char[6] = BATOTO_SEARCH_INDEX_END_CHAR;
   sn->provider.replace_str = strdup(BATOTO_REPLACE_STR);
   sn->provider.data_cb = BATOTO_DATA_CB;
   sn->provider.init_cb = BATOTO_INIT_CB;
}

#ifndef EMG_H
#define EMG_H

#include <Evas.h>
#include <Elementary.h>
#include <Ecore_Con.h>

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

#define DBG(...)            EINA_LOG_DOM_DBG(_emg_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(_emg_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(_emg_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(_emg_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(_emg_log_dom, __VA_ARGS__)

extern int _emg_log_dom;


#define WEIGHT evas_object_size_hint_weight_set
#define ALIGN evas_object_size_hint_align_set
#define EXPAND(X) WEIGHT((X), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND)
#define FILL(X) ALIGN((X), EVAS_HINT_FILL, EVAS_HINT_FILL)

#define PROVIDER_SEARCH_SETUP(SN, PROVIDER) \
do { \
   (SN)->provider.url = eina_stringshare_add(PROVIDER##_URL); \
   (SN)->provider.search_url = eina_stringshare_add(PROVIDER##_SEARCH_URL); \
   (SN)->provider.search_index = PROVIDER##_SEARCH_INDEX; \
   (SN)->provider.search_name_count = PROVIDER##_SEARCH_INDEX_NAME_COUNT; \
   (SN)->provider.index_start[0] = PROVIDER##_SEARCH_INDEX_START; \
   (SN)->provider.index_char[0] = PROVIDER##_SEARCH_INDEX_START_CHAR; \
   (SN)->provider.index_start[1] = PROVIDER##_SEARCH_INDEX_POST_IMAGE; \
   (SN)->provider.index_char[1] = PROVIDER##_SEARCH_INDEX_POST_IMAGE_CHAR; \
   (SN)->provider.index_start[2] = PROVIDER##_SEARCH_INDEX_POST_HREF; \
   (SN)->provider.index_char[2] = PROVIDER##_SEARCH_INDEX_POST_HREF_CHAR; \
   (SN)->provider.index_start[3] = PROVIDER##_SEARCH_INDEX_CHAP; \
   (SN)->provider.index_char[3] = PROVIDER##_SEARCH_INDEX_CHAP_CHAR; \
   (SN)->provider.index_start[4] = PROVIDER##_SEARCH_INDEX_TAGS; \
   (SN)->provider.index_char[4] = PROVIDER##_SEARCH_INDEX_TAGS_CHAR; \
   (SN)->provider.index_start[5] = PROVIDER##_SEARCH_INDEX_END; \
   (SN)->provider.index_char[5] = PROVIDER##_SEARCH_INDEX_END_CHAR; \
   (SN)->provider.data_cb = PROVIDER##_DATA_CB; \
   (SN)->provider.init_cb = PROVIDER##_INIT_CB; \
} while (0)

#define IDENTIFIER_COMIC_PAGE_IMAGE 701
#define IDENTIFIER_COMIC_PAGE 700

#define IDENTIFIER_COMIC_CHAPTER 662
#define IDENTIFIER_COMIC_SERIES 661
#define IDENTIFIER_COMIC_IMAGE 660

#define IDENTIFIER_SEARCH_NAME 11
#define IDENTIFIER_SEARCH_IMAGE 10



#define DEFAULT_PAGE_READAHEAD 5
#define DEFAULT_PAGE_READBEHIND 2

typedef struct Search_Name Search_Name;
typedef struct Comic_Series Comic_Series;
typedef struct Search_Result Search_Result;
typedef struct Comic_Chapter Comic_Chapter;
typedef struct Comic_Page Comic_Page;
typedef struct Comic_Provider Comic_Provider;
typedef void (*Provider_Data_Cb)(void *);
typedef void (*Provider_Init_Cb)(void *);

typedef enum EMG_View
{
   EMG_VIEW_SEARCH,
   EMG_VIEW_SERIES,
   EMG_VIEW_READER
} EMG_View;

typedef struct Search_Window
{
   Evas_Object *box;
   Evas_Object *fr;
   Evas_Object *entry;
   Evas_Object *progress;
   Evas_Object *list;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *nf_it;

   Eina_List *searches;
   unsigned int running;
   Elm_Genlist_Item_Class itc;
} Search_Window;

typedef struct Comic_View
{
   Evas_Object *nf;
   Evas_Object *prev, *next;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *nf_it;
   Comic_Chapter *cc;
} Comic_View;

typedef struct Series_View
{
   Evas_Object *scr;
   Evas_Object *box;
   Evas_Object *img; /* series img */
   Evas_Object *title_lbl;
   Evas_Object *fr;
   Evas_Object *desc_lbl;
   Evas_Object *hbox;
   Evas_Object *auth_lbl;
   Evas_Object *art_lbl;
   Evas_Object *hbox2;
   Evas_Object *chap_lbl;
   Evas_Object *year_lbl;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *nf_it;

   Evas_Object *list; /* ch list */
   Comic_Series *cs;
   Elm_Genlist_Item_Class itc;
} Series_View;

typedef struct EMG
{
   Search_Window sw;
   Comic_View cv;
   Series_View sv;
   Evas_Object *win;
   Evas_Object *box;
   Evas_Object *hbox;
   Evas_Object *tb;
   Evas_Object *nf;
   Eina_List *series;
   Eina_List *providers;
   EMG_View view;
} EMG;

struct Comic_Provider
{
   const char *url;
   const char *search_url;
   size_t search_index;
   unsigned int search_name_count;
   unsigned int index_start[10];
   char index_char[10];
   Provider_Data_Cb data_cb;
   Provider_Init_Cb init_cb;
};

/* a user search by name */
struct Search_Name
{
   unsigned int identifier;
   EMG *e;
   Ecore_Con_Url *ecu;
   Eina_Strbuf *buf;
   const char *name;
   unsigned int namelen;
   unsigned int idx[2]; /* position, iterator */
   /* inherited from provider */
   Comic_Provider provider;
   Eina_Inlist *results;
   unsigned int result_count;
   Eina_Bool done : 1;
};

typedef struct Comic_Image
{
   unsigned int identifier;
   const char *imgurl;
   Eina_Binbuf *buf;
   Ecore_Con_Url *ecu;
   void *parent;
} Comic_Image;

struct Search_Result
{
   unsigned int identifier;
   EINA_INLIST;
   EMG *e;
   unsigned int *search; /* deref to determine parent type */
   Elm_Object_Item *it; /* list item */
   const char *provider_url;
   const char *name;
   unsigned int namelen;
   char *href;
   unsigned int total;
   Eina_List *tags;
   unsigned int tags_len;
   Comic_Image image;
};

struct Comic_Page
{
   unsigned int identifier;
   EINA_INLIST;
   const char *href;
   unsigned int hreflen;
   unsigned int number;
   Evas_Object *obj;
   Elm_Object_Item *nf_it;
   Comic_Chapter *cc;
   Comic_Image image;
   unsigned int idx[2]; /* position, iterator */
   Comic_Provider provider;
   Eina_Strbuf *buf;
   Ecore_Con_Url *ecu;
   Eina_Bool done : 1;
};

struct Comic_Chapter
{
   unsigned int identifier;
   EINA_INLIST;
   Comic_Page *current;
   Comic_Series *cs;
   double number; /* chapter number */
   unsigned int pages_fetched;
   const char *name;
   const char *href;
   const char *date;
   Eina_Inlist *pages;
   unsigned int page_count;
   Eina_Bool decimal : 1;
   Eina_Bool done : 1;
};

struct Comic_Series
{
   unsigned int identifier;
   EMG *e;
   Eina_Strbuf *buf;
   unsigned int total;
   char *desc;
   const char *name;
   const char *alt_name;
   unsigned int year;
   const char *author;
   const char *artist;
   unsigned int namelen;
   Eina_Inlist *chapters;
   Eina_Inlist *populate_job;
   Ecore_Con_Url *ecu;
   Comic_Chapter *current;
   Comic_Image image;
   unsigned int idx[2]; /* position, iterator */
   Comic_Provider provider;
   Eina_Bool done : 1;
   Eina_Bool completed : 1;
};

void search_name_free(Search_Name *sn);
void search_name_parser(Search_Name *sn);
char *search_name_list_text_cb(Search_Result *sr, Evas_Object *obj, const char *part);
Evas_Object *search_name_list_pic_cb(Search_Result *sr, Evas_Object *obj, const char *part);
void search_name_list_init(EMG *e, Evas_Object *list);

void search_view_clear(EMG *e);
void search_name_create(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
void search_view_count_update(Search_Name *sn);
void search_view_show(EMG *e, Evas_Object *obj, Elm_Object_Item *event_info);
void search_result_pick(EMG *e, Evas_Object *obj __UNUSED__, Elm_Object_Item *it);

void search_result_free(Search_Result *sr);
Search_Result *search_result_add(Search_Name *sn);
void search_result_tag_add(Search_Result *sr, const char *index_start, const char *tag);

void comic_view_readahead_ensure(EMG *e);
void comic_view_readbehind_ensure(EMG *e);
void comic_view_page_set(EMG *e, Comic_Page *cp);
void comic_view_chapter_set(EMG *e, Comic_Chapter *cc);
void comic_view_show(EMG *e, Evas_Object *obj, Elm_Object_Item *event_info);
void comic_view_page_prev(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
void comic_view_page_next(EMG *e, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);

void series_view_populate(Comic_Series *cs);
void series_view_clear(EMG *e);
void series_view_show(EMG *e, Evas_Object *obj, Elm_Object_Item *event_info);
char *series_view_list_text_cb(Comic_Chapter *cc, Evas_Object *obj, const char *part);
void series_view_author_set(EMG *e, Comic_Series *cs);
void series_view_artist_set(EMG *e, Comic_Series *cs);
void series_view_chapters_set(EMG *e, Comic_Series *cs);
void series_view_year_set(EMG *e, Comic_Series *cs);
void series_view_title_set(EMG *e, Comic_Series *cs);
void series_view_desc_set(EMG *e, Comic_Series *cs);
void series_view_image_set(EMG *e, Eina_Binbuf *buf);
void series_view_list_init(EMG *e, Evas_Object *list);

Comic_Series *comic_series_find(EMG *e, const char *name);
Comic_Series *comic_series_new(Search_Result *sr);
void comic_series_parser(Comic_Series *cs);

Comic_Chapter *comic_chapter_new(Comic_Series *cs);
void comic_chapter_clear(Comic_Chapter *cc);
Comic_Chapter *comic_chapter_prev_get(Comic_Chapter *cc);
Comic_Chapter *comic_chapter_next_get(Comic_Chapter *cc);

Comic_Page *comic_page_new(Comic_Chapter *cc, unsigned int id);
void comic_page_fetch(Comic_Page *page);
void comic_page_parser(Comic_Page *cp);
Comic_Page *comic_page_prev_get(Comic_Page *cp);
Comic_Page *comic_page_next_get(Comic_Page *cp);
Eina_Bool comic_page_current(Comic_Page *cp);

void mangareader_search_init_cb(Search_Name *sn);

#endif

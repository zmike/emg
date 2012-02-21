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


#define IDENTIFIER_COMIC_CHAPTER 662
#define IDENTIFIER_COMIC_SERIES 661
#define IDENTIFIER_COMIC_IMAGE 660
#define IDENTIFIER_SEARCH_NAME 11
#define IDENTIFIER_SEARCH_IMAGE 10

typedef struct Search_Name Search_Name;
typedef struct Comic_Series Comic_Series;
typedef struct Search_Result Search_Result;
typedef struct Comic_Chapter Comic_Chapter;
typedef struct Comic_Provider Comic_Provider;
typedef void (*Provider_Data_Cb)(void *data);
typedef void (*Provider_Init_Cb)(void *);

typedef struct Search_Window
{
   Evas_Object *box;
   Evas_Object *fr;
   Evas_Object *entry;
   Evas_Object *progress;
   Evas_Object *list;
   Elm_Object_Item *tb_it;

   Eina_List *searches;
   unsigned int running;
   Elm_Genlist_Item_Class itc;
} Search_Window;

typedef struct Comic_View
{
   Evas_Object *img;
   Elm_Object_Item *tb_it;
   Comic_Series *cs;
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
} EMG;

struct Comic_Provider
{
   const char *url;
   const char *search_url;
   size_t search_index;
   unsigned int search_name_count;
   unsigned int index_start[10];
   char index_char[10];
   unsigned int index_data;
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

struct Comic_Chapter
{
   unsigned int identifier;
   EINA_INLIST;
   Comic_Series *cs;
   double number; /* chapter number */
   const char *name;
   const char *href;
   const char *date;
   Eina_List *imgs; /* binbufs */
   Eina_Bool decimal : 1;
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
   Ecore_Con_Url *ecu;
   Comic_Image image;
   unsigned int idx[2]; /* position, iterator */
   Comic_Provider provider;
   Eina_Bool done : 1;
   Eina_Bool completed : 1;
   Eina_Bool current : 1;
};

void search_name_free(Search_Name *sn);
void search_name_parser(Search_Name *sn);
char *search_name_list_text_cb(Search_Result *sr, Evas_Object *obj, const char *part);
Evas_Object *search_name_list_pic_cb(Search_Result *sr, Evas_Object *obj, const char *part);
void search_name_list_init(EMG *e, Evas_Object *list);
void search_view_show(EMG *e, Evas_Object *obj, Elm_Object_Item *event_info);

void search_result_free(Search_Result *sr);
Search_Result *search_result_add(Search_Name *sn);
void search_result_tag_add(Search_Result *sr, const char *index_start, const char *tag);

void comic_view_show(EMG *e, Evas_Object *obj, Elm_Object_Item *event_info);

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

Comic_Series *comic_series_find(EMG *e, const char *name);
Comic_Series *comic_series_create(Search_Result *sr);
void comic_series_parser(Comic_Series *cs);

Comic_Chapter *comic_chapter_new(Comic_Series *cs);

void mangareader_search_init_cb(Search_Name *sn);

#endif

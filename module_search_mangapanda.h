#include "emg.h"

/* actual page data offsets */
#define MANGAPANDA_SEARCH_INDEX 12509 /* off by +2 for some reason, but it works */
#define MANGAPANDA_SEARCH_INDEX_NAME_COUNT 1
#define MANGAPANDA_SEARCH_INDEX_START 145
#define MANGAPANDA_SEARCH_INDEX_START_CHAR '\''
#define MANGAPANDA_SEARCH_INDEX_POST_IMAGE 83
#define MANGAPANDA_SEARCH_INDEX_POST_IMAGE_CHAR '"'
#define MANGAPANDA_SEARCH_INDEX_POST_HREF 2
#define MANGAPANDA_SEARCH_INDEX_POST_HREF_CHAR '<'
#define MANGAPANDA_SEARCH_INDEX_CHAP 51
#define MANGAPANDA_SEARCH_INDEX_CHAP_CHAR '<'
#define MANGAPANDA_SEARCH_INDEX_TAGS 88
#define MANGAPANDA_SEARCH_INDEX_TAGS_CHAR '<'
#define MANGAPANDA_SEARCH_INDEX_END 54
#define MANGAPANDA_SEARCH_INDEX_END_CHAR 0
#define MANGAPANDA_DATA_CB (Provider_Data_Cb)mangapanda_search_name_cb
#define MANGAPANDA_INIT_CB (Provider_Init_Cb)mangapanda_series_init_cb

#define MANGAPANDA_SERIES_INDEX 2485
#define MANGAPANDA_SERIES_INDEX_NAME_COUNT 8
#define MANGAPANDA_SERIES_INDEX_START 0
#define MANGAPANDA_SERIES_INDEX_START_CHAR '"'
#define MANGAPANDA_SERIES_INDEX_POST_IMAGE 214
#define MANGAPANDA_SERIES_INDEX_POST_IMAGE_NAME_COUNT 3
#define MANGAPANDA_SERIES_INDEX_POST_IMAGE_CHAR '<'
#define MANGAPANDA_SERIES_INDEX_ALT_NAME 69
#define MANGAPANDA_SERIES_INDEX_ALT_NAME_CHAR '<'
#define MANGAPANDA_SERIES_INDEX_YEAR 60
#define MANGAPANDA_SERIES_INDEX_YEAR_CHAR '<'
#define MANGAPANDA_SERIES_INDEX_COMPLETED 60
#define MANGAPANDA_SERIES_INDEX_COMPLETED_CHAR '<'
#define MANGAPANDA_SERIES_INDEX_AUTHOR 60
#define MANGAPANDA_SERIES_INDEX_AUTHOR_CHAR '<'
#define MANGAPANDA_SERIES_INDEX_ARTIST 60
#define MANGAPANDA_SERIES_INDEX_ARTIST_CHAR '<'
#define MANGAPANDA_SERIES_INDEX_JUMP 67
#define MANGAPANDA_SERIES_INDEX_JUMP_CHAR 0

#define MANGAPANDA_PAGE_INDEX 1699
#define MANGAPANDA_PAGE_INDEX_NAME_COUNT 10
#define MANGAPANDA_PAGE_INDEX_START 0
#define MANGAPANDA_PAGE_INDEX_START_CHAR ']'
#define MANGAPANDA_PAGE_INDEX_JUMP 24
#define MANGAPANDA_PAGE_INDEX_JUMP_CHAR ']'
#define MANGAPANDA_PAGE_INDEX_NEXT 5
#define MANGAPANDA_PAGE_INDEX_NEXT_CHAR '\''
#define MANGAPANDA_PAGE_INDEX_PREV 25
#define MANGAPANDA_PAGE_INDEX_PREV_CHAR '\''
#define MANGAPANDA_PAGE_INDEX_IMG 25
#define MANGAPANDA_PAGE_INDEX_IMG_CHAR '\''

#define MANGAPANDA_REPLACE_STR "+"

#define MANGAPANDA_SEARCH_URL "http://www.mangapanda.com/search/?w=%s"
#define MANGAPANDA_URL "http://www.mangapanda.com"

#define MANGAPANDA_PROVIDER_PRIORITY 5
#include "emg.h"

/* actual page data offsets */
#define BATOTO_SEARCH_INDEX 33284
#define BATOTO_SEARCH_INDEX_START 73
#define BATOTO_SEARCH_INDEX_START_CHAR '"'
#define BATOTO_SEARCH_INDEX_POST_HREF 93
#define BATOTO_SEARCH_INDEX_POST_HREF_CHAR '<'
#define BATOTO_SEARCH_INDEX_POST_NAME 59
#define BATOTO_SEARCH_INDEX_POST_NAME_CHAR '<'
#define BATOTO_SEARCH_INDEX_POST_AUTHOR 419
#define BATOTO_SEARCH_INDEX_POST_AUTHOR_CHAR '<'
#define BATOTO_SEARCH_INDEX_POST_VIEWS 48
#define BATOTO_SEARCH_INDEX_POST_VIEWS_CHAR '<'
#define BATOTO_SEARCH_INDEX_POST_FOLLOWS 47
#define BATOTO_SEARCH_INDEX_POST_FOLLOWS_CHAR '<'
#define BATOTO_SEARCH_INDEX_END 12
#define BATOTO_SEARCH_INDEX_END_CHAR 0
#define BATOTO_DATA_CB (Provider_Data_Cb)batoto_search_name_cb
#define BATOTO_INIT_CB (Provider_Init_Cb)batoto_series_init_cb

#define BATOTO_SERIES_INDEX 17000 /* this is a huge pain to parse, so fuck it */
#define BATOTO_SERIES_INDEX_START 1
#define BATOTO_SERIES_INDEX_START_CHAR 0
#define BATOTO_SERIES_INDEX_START_STR "ipsBox"
#define BATOTO_SERIES_INDEX_IMAGE 130
#define BATOTO_SERIES_INDEX_IMAGE_CHAR '"'
#define BATOTO_SERIES_INDEX_ALT_NAME 470
#define BATOTO_SERIES_INDEX_ALT_NAME_CHAR '<'
#define BATOTO_SERIES_INDEX_AUTHOR 212
#define BATOTO_SERIES_INDEX_AUTHOR_CHAR '"'
#define BATOTO_SERIES_INDEX_ARTIST 215
#define BATOTO_SERIES_INDEX_ARTIST_CHAR '"'
#define BATOTO_SERIES_INDEX_POST_ARTIST 157
#define BATOTO_SERIES_INDEX_POST_ARTIST_CHAR '\r'
#define BATOTO_SERIES_INDEX_PRE_COMPLETED 203
#define BATOTO_SERIES_INDEX_PRE_COMPLETED_CHAR '>'
#define BATOTO_SERIES_INDEX_DESC 164
#define BATOTO_SERIES_INDEX_DESC_CHAR '<'
/* rough estimates */
#define BATOTO_SERIES_INDEX_JUMP 31000
#define BATOTO_SERIES_INDEX_TRAILING 50000
#define BATOTO_SERIES_INDEX_ROW_SIZE 930

#define BATOTO_PAGE_INDEX 4000
/* estimates */
#define BATOTO_PAGE_INDEX_CHAPTER_JUMP 120


#define BATOTO_REPLACE_STR "%20"

#define BATOTO_SEARCH_URL "http://www.batoto.net/search?name=%s&name_cond=c"
#define BATOTO_URL "http://www.batoto.net"

#define BATOTO_PROVIDER_PRIORITY 9

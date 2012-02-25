#include "emg.h"

const char *
util_markup_to_utf8(const char *start, const char *p)
{
   char *buf;
   const char *ret;

   buf = strndupa(start, p - start);
   buf = evas_textblock_text_markup_to_utf8(NULL, buf);
   ret = eina_stringshare_add(buf);
   free(buf);
   return ret;
}

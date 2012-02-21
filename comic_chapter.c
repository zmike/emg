#include "emg.h"

Comic_Chapter *
comic_chapter_new(Comic_Series *cs)
{
   Comic_Chapter *cc;

   cc = calloc(1, sizeof(Comic_Chapter));
   cc->cs = cs;
   cc->identifier = IDENTIFIER_COMIC_CHAPTER;
   cs->chapters = eina_inlist_append(cs->chapters, EINA_INLIST_GET(cc));
   return cc;
}

#ifndef MEDIA_LIST_ITEM_H
#define MEDIA_LIST_ITEM_H

#include <time.h>

struct MediaListItem {
	time_t                      st_mtime;
	int                         size;
	char                        type[8];
	char                        *path;
	char                        *name;
	struct MediaListItem        *next;
};

#endif	//MEDIA_LIST_ITEM_H

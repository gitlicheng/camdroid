
#ifndef LIB_M3U_H
#define LIB_M3U_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct PLAYLIST_TYPE
    {
        char* encoding;
        char* contentType;
        char* extension;
    }PlaylistType;

    typedef struct ENCRYPTION_INFO EncryptionInfo;
    struct ENCRYPTION_INFO
    {
        char* uri;
        char* method;

        char* (*getURI)(EncryptionInfo* encryptionInfo);
        char* (*getMethod)(EncryptionInfo* encryptionInfo);
    };


    typedef struct PLAYLIST_INFO PlaylistInfo;
    struct PLAYLIST_INFO
    {
        int   programId;
        int   bandWidth;
        char* codec;

        int   (*getProgramId)(PlaylistInfo* playlistInfo);
        int   (*getBandWidth)(PlaylistInfo* playlistInfo);
        char* (*getCodec)(PlaylistInfo* playlistInfo);
    };


    typedef struct ELEMENT Element;
    struct ELEMENT
    {
        PlaylistInfo*   playlistInfo;
        EncryptionInfo* encyptionInfo;
        int             duration;
        char*           uri;
        char*           title;
        unsigned int    programDate;

        Element*        next;

        char*           (*getTitle)(Element* element);
        int             (*getDuration)(Element* element);
        char*           (*getURI)(Element* element);
        int             (*isEncrypted)(Element* element);
        int             (*isPlaylist)(Element* element);
        int             (*isMedia)(Element* element);
        EncryptionInfo* (*getEncryptionInfo)(Element* element);
        PlaylistInfo*   (*getPlaylistInfo)(Element* element);
        int             (*getProgramDate)(Element* element);
    };

    typedef struct PLAYLIST Playlist;
    struct PLAYLIST
    {
        PlaylistType    type;
        Element*        elementList;
        Element*        curElement;
        int             endSet;
        int             targetDuration;
        int             mediaSequenceNumber;

        int         (*getTargetDuration)(Playlist* playlist);
        Element*    (*iterator)(Playlist* playlist);
        void        (*iteratorReset)(Playlist* playlist);
        Element*    (*getElementList)(Playlist* playlist);
        int         (*isEndSet)(Playlist* playlist);
        int         (*getMediaSequenceNumber)(Playlist* playlist);

    };

    Playlist* CreatePlaylist(char* m3uBuffer, int m3uBufLen);
    void      DeletePlaylist(Playlist* playlist);

#ifdef __cplusplus
}
#endif

#endif


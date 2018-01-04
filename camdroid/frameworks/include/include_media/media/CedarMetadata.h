/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
   Class to hold the media's metadata.  Metadata are used
   for human consumption and can be embedded in the media (e.g
   shoutcast) or available from an external source. The source can be
   local (e.g thumbnail stored in the DB) or remote.

   Metadata is like a Bundle. It is sparse and each key can occur at
   most once. The key is an integer and the value is the actual metadata.

   The caller is expected to know the type of the metadata and call
   the right get* method to fetch its value.
   
   @hide
 */
#ifndef __CEDAR_METADATA_H__
#define __CEDAR_METADATA_H__

#include <utils/SortedVector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <binder/Parcel.h>


namespace android {

class CedarMetadata
{
public:
    // The metadata are keyed using integers rather than more heavy
    // weight strings. We considered using Bundle to ship the metadata
    // between the native layer and the java layer but dropped that
    // option since keeping in sync a native implementation of Bundle
    // and the java one would be too burdensome. Besides Bundle uses
    // String for its keys.
    // The key range [0 8192) is reserved for the system.
    //
    // We manually serialize the data in Parcels. For large memory
    // blob (bitmaps, raw pictures) we use MemoryFile which allow the
    // client to make the data purge-able once it is done with it.
    //

    enum
    {

        ANY = 0,  // Never used for metadata returned, only for filtering.
                                          // Keep in sync with kAny in MediaPlayerService.cpp

        // Playback capabilities.
        /**
         * Indicate whether the media can be paused
         */
        PAUSE_AVAILABLE         = 1, // Boolean
        /**
         * Indicate whether the media can be backward seeked
         */
        SEEK_BACKWARD_AVAILABLE = 2, // Boolean
        /**
         * Indicate whether the media can be forward seeked
         */
        SEEK_FORWARD_AVAILABLE  = 3, // Boolean
        /**
         * Indicate whether the media can be seeked
         */
        SEEK_AVAILABLE          = 4, // Boolean

        //  TODO: Should we use numbers compatible with the metadata retriever?
        TITLE                   = 5, // String
        COMMENT                 = 6, // String
        COPYRIGHT               = 7, // String
        ALBUM                   = 8, // String
        ARTIST                  = 9, // String
        AUTHOR                  = 10, // String
        COMPOSER                = 11, // String
        GENRE                   = 12, // String
        DATE                    = 13, // Date
        DURATION                = 14, // Integer(millisec)
        CD_TRACK_NUM            = 15, // Integer 1-based
        CD_TRACK_MAX            = 16, // Integer
        RATING                  = 17, // String
        ALBUM_ART               = 18, // byte[]
        VIDEO_FRAME             = 19, // Bitmap

        BIT_RATE                = 20, // Integer, Aggregate rate of
                                                              // all the streams in bps.
        AUDIO_BIT_RATE          = 21, // Integer, bps
        VIDEO_BIT_RATE          = 22, // Integer, bps
        AUDIO_SAMPLE_RATE       = 23, // Integer, Hz
        VIDEO_FRAME_RATE        = 24, // Integer, Hz

        // See RFC2046 and RFC4281.
        MIME_TYPE               = 25, // String
        AUDIO_CODEC             = 26, // String
        VIDEO_CODEC             = 27, // String

        VIDEO_HEIGHT            = 28, // Integer
        VIDEO_WIDTH             = 29, // Integer
        NUM_TRACKS              = 30, // Integer
        DRM_CRIPPLED            = 31, // Boolean

        LAST_SYSTEM = 31,
        FIRST_CUSTOM = 8192,
    };

    // Shorthands to set the MediaPlayer's metadata filter.
    //public static final Set<Integer> MATCH_NONE = Collections.EMPTY_SET;
    //static const SortedVector<int> MATCH_NONE;  // = Collections.EMPTY_SET;
    
    //public static final Set<Integer> MATCH_ALL = Collections.singleton(ANY);
    //static const SortedVector<int> MATCH_ALL;   // = Collections.singleton(ANY);

    enum
    {
        STRING_VAL     = 1,
        INTEGER_VAL    = 2,
        BOOLEAN_VAL    = 3,
        LONG_VAL       = 4,
        DOUBLE_VAL     = 5,
        DATE_VAL       = 6,
        BYTE_ARRAY_VAL = 7,
        // FIXME: misses a type for shared heap is missing (MemoryFile).
        // FIXME: misses a type for bitmaps.
        LAST_TYPE = 7,
    };

private:
    static const int kInt32Size = 4;
    static const int kMetaHeaderSize = 2 * kInt32Size; //  size + marker
    static const int kRecordHeaderSize = 3 * kInt32Size; // size + id + type

    static const int kMetaMarker = 0x4d455441;  // 'M' 'E' 'T' 'A'

    // After a successful parsing, set the parcel with the serialized metadata.
    Parcel *mParcel;

    // Map to associate a Metadata key (e.g TITLE) with the offset of
    // the record's payload in the parcel.
    // Used to look up if a key was present too.
    // Key: Metadata ID
    // Value: Offset of the metadata type field in the record.
//    private final HashMap<Integer, Integer> mKeyToPosMap =
//            new HashMap<Integer, Integer>();
    KeyedVector<int, int> mKeyToPosMap; // = new HashMap<Integer, Integer>();


public:
    CedarMetadata();
	~CedarMetadata();

private:
    /**
     * Go over all the records, collecting metadata keys and records'
     * type field offset in the Parcel. These are stored in
     * mKeyToPosMap for latter retrieval.
     * Format of a metadata record:
     <pre>
                         1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     record size                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     metadata key                              |  // TITLE
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     metadata type                             |  // STRING_VAL
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                .... metadata payload ....                     |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     </pre>
     * @param parcel With the serialized records.
     * @param bytesLeft How many bytes in the parcel should be processed.
     * @return false if an error occurred during parsing.
     */
    bool scanAllRecords(Parcel& parcel, int bytesLeft);
    /**
     * Check a parcel containing metadata is well formed. The header
     * is checked as well as the individual records format. However, the
     * data inside the record is not checked because we do lazy access
     * (we check/unmarshall only data the user asks for.)
     *
     * Format of a metadata parcel:
     <pre>
                         1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     metadata total size                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |     'M'       |     'E'       |     'T'       |     'A'       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                .... metadata records ....                     |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     </pre>
     *
     * @param parcel With the serialized data. Metadata keeps a
     *               reference on it to access it later on. The caller
     *               should not modify the parcel after this call (and
     *               not call recycle on it.)
     * @return false if an error occurred.
     * {@hide}
     */
public:
    bool parse(Parcel& parcel);
    /**
     * @return The set of metadata ID found.
     */
    SortedVector<int>& keySet();
    /**
     * @return true if a value is present for the given key.
     */
    bool has(const int metadataId);

    // Accessors.
    // Caller must make sure the key is present using the {@code has}
    // method otherwise a RuntimeException will occur.

    String8 getString(const int key);

    int getInt(const int key);
    /**
     * Get the boolean value indicated by key
     */
    bool getBoolean(const int key);

    int64_t getLong(const int key);
    double getDouble(const int key);
    int getByteArray(const int key, /*out*/char **p, /*out*/ int *size);

    //public Date getDate(const int key);
    /**
     * @return the last available system metadata id. Ids are
     *         1-indexed.
     */
    static int lastSytemId();

    /**
     * @return the first available cutom metadata id.
     */
    static int firstCustomId();

    /**
     * @return the last value of known type. Types are 1-indexed.
     */
    static int lastType();

    /**
     * Check val is either a system id or a custom one.
     * @param val Metadata key to test.
     * @return true if it is in a valid range.
     **/
private:
    bool checkMetadataId(const int val);
    /**
     * Check the type of the data match what is expected.
     */
    void checkType(const int key, const int expectedType);
};

};

#endif


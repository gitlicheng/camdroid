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
#include <CedarMetadata.h>

 
namespace android {

CedarMetadata::CedarMetadata() :
	mParcel(NULL),
	mKeyToPosMap()
{
}

CedarMetadata::~CedarMetadata()
{
}


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
bool CedarMetadata::scanAllRecords(Parcel& parcel, int bytesLeft)
{
	int recCount = 0;
	bool error = false;
	
	mKeyToPosMap.clear();
	while (bytesLeft > kRecordHeaderSize) {
		const int start = parcel.dataPosition();
		const int size = parcel.readInt32();

		if (size <= kRecordHeaderSize) {  // at least 1 byte should be present.
			ALOGE("Record is too short");
			error = true;
			break;
		}

		// Check the metadata key.
		static int metadataId = parcel.readInt32();
		if (!checkMetadataId(metadataId)) {
			error = true;
			break;
		}

		// Store the record offset which points to the type
		// field so we can later on read/unmarshall the record
		// payload.
		if (mKeyToPosMap.indexOfKey(metadataId) >= 0) {
			ALOGE("Duplicate metadata ID found");
			error = true;
			break;
		}

		mKeyToPosMap.add(metadataId, parcel.dataPosition());

		// Check the metadata type.
		const int metadataType = parcel.readInt32();
		if (metadataType <= 0 || metadataType > LAST_TYPE) {
			ALOGE("Invalid metadata type %d", metadataType);
			error = true;
			break;
		}

		// Skip to the next one.
		parcel.setDataPosition(start + size);
		bytesLeft -= size;
		++recCount;
	}

	if (0 != bytesLeft || error) {
		ALOGE("Ran out of data or error on record %d", recCount);
		mKeyToPosMap.clear();
		return false;
	} else {
		return true;
	}
}
	
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
 */
bool CedarMetadata::parse(Parcel& parcel)
{
	if (parcel.dataAvail() < (size_t)kMetaHeaderSize) {
		ALOGE("Not enough data %d", parcel.dataAvail());
		return false;
	}

	const int pin = parcel.dataPosition();  // to roll back in case of errors.
	const int size = parcel.readInt32();

	// The extra kInt32Size below is to account for the int32 'size' just read.
	if (parcel.dataAvail() + kInt32Size < (size_t)size || size < kMetaHeaderSize) {
		ALOGE("Bad size %d avail %d position %d", size, parcel.dataAvail(), pin);
		parcel.setDataPosition(pin);
		return false;
	}

	// Checks if the 'M' 'E' 'T' 'A' marker is present.
	const int kShouldBeMetaMarker = parcel.readInt32();
	if (kShouldBeMetaMarker != kMetaMarker ) {
		ALOGE("Marker missing");
		parcel.setDataPosition(pin);
		return false;
	}

	// Scan the records to collect metadata ids and offsets.
	if (!scanAllRecords(parcel, size - kMetaHeaderSize)) {
		parcel.setDataPosition(pin);
		return false;
	}
	mParcel = &parcel;
	return true;
}

#if 0
/**
 * @return The set of metadata ID found.
 */
SortedVector<int>& CedarMetadata::keySet()
{
	return NULL;
}
#endif
	
/**
 * @return true if a value is present for the given key.
 */
bool CedarMetadata::has(const int metadataId)
{
	if (!checkMetadataId(metadataId)) {
		ALOGE("Invalid key: %d", metadataId);
		return false;
	}
	return mKeyToPosMap.indexOfKey(metadataId) >= 0;
}

// Accessors.
// Caller must make sure the key is present using the {@code has}
// method otherwise a RuntimeException will occur.
String8 CedarMetadata::getString(const int key)
{
	checkType(key, STRING_VAL);
	return mParcel->readString8();
}

int CedarMetadata::getInt(const int key)
{
	checkType(key, INTEGER_VAL);
	return mParcel->readInt32();
}

bool CedarMetadata::getBoolean(const int key)
{
	checkType(key, BOOLEAN_VAL);
	return mParcel->readInt32() == 1;
}

int64_t CedarMetadata::getLong(const int key)
{
	checkType(key, LONG_VAL);
	return mParcel->readInt64();
}

double CedarMetadata::getDouble(const int key)
{
	checkType(key, DOUBLE_VAL);
	return mParcel->readDouble();
}

int CedarMetadata::getByteArray(const int key, /*out*/char **p, /*out*/ int *size)
{
	checkType(key, BYTE_ARRAY_VAL);
	int32_t len = mParcel->readInt32();
	*p = (char *)mParcel->readInplace(len);
	return 0;
}

/**
 * @return the last available system metadata id. Ids are 1-indexed.
 */
int CedarMetadata::lastSytemId()
{
	return LAST_SYSTEM;
}


/**
 * @return the first available cutom metadata id.
 */
int CedarMetadata::firstCustomId()
{
	return FIRST_CUSTOM;
}


/**
 * @return the last value of known type. Types are 1-indexed.
 */
int CedarMetadata::lastType()
{
	return LAST_TYPE;
}


/**
 * Check val is either a system id or a custom one.
 * @param val Metadata key to test.
 * @return true if it is in a valid range.
 **/
bool CedarMetadata::checkMetadataId(const int val)
{
	if (val <= ANY || (LAST_SYSTEM < val && val < FIRST_CUSTOM)) {
		ALOGE("Invalid metadata ID %d", val);
		return false;
	}
	return true;
}

/**
 * Check the type of the data match what is expected.
 */
void CedarMetadata::checkType(const int key, const int expectedType)
{
	const int pos = mKeyToPosMap.valueFor(key);

	mParcel->setDataPosition(pos);

	const int type = mParcel->readInt32();
	if (type != expectedType) {
		ALOGE("Wrong type %d but got %d", expectedType, type);
	}
}

};


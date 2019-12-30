// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "KtxFile.h"

using namespace nv;

static const uint8 fileIdentifier[12] = {
    0xAB, 0x4B, 0x54, 0x58,
    0x20, 0x31, 0x31, 0xBB,
    0x0D, 0x0A, 0x1A, 0x0A
};


KtxHeader::KtxHeader() {
    memcpy(identifier, fileIdentifier, 12);

    endianness = 0x04030201;

    glType = 0;
    glTypeSize = 1;
    glFormat = 0;
    glInternalFormat = KTX_RGBA;
    glBaseInternalFormat = KTX_RGBA;
    pixelWidth = 0;
    pixelHeight = 0;
    pixelDepth = 0;
    numberOfArrayElements = 0;
    numberOfFaces = 1;
    numberOfMipmapLevels = 0;
    bytesOfKeyValueData = 0;
}


Stream & operator<< (Stream & s, DDSHeader & header) {
    s.serialize(header.identifier, 12);
    s << header.endiannes << header.glType << header.glTypeSize << header.glFormat << header.glInternalFormat << header.glBaseInternalFormat;
    s << header.pixelWidth << header.pixelHeight << header.pixelDepth;
    s << header.numberOfArrayElements << header.numberOfFaces << header.numberOfMipmapLevels;
    s << header.bytesOfKeyValueData;
    return s;
}


KtxFile::KtxFile() {
}
KtxFile::~KtxFile() {
}

void KtxFile::addKeyValue(const char * key, const char * value) {
    keyArray.append(key);
    valueArray.append(value);
    bytesOfKeyValueData += strlen(key) + 1 + strlen(value) + 1;
}


Stream & operator<< (Stream & s, KtxFile & file) {
    s << header;

    if (s.isSaving()) {

        int keyValueCount = keyArray.count();
        for (int i = 0; i < keyValueCount; i++) {
            const String & key = keyArray[i];
            const String & value = valueArray[i];
            uint keySize = key.length() + 1;
            uint valueSize = value.length() + 1;
            uint keyValueSize = keySize + valueSize;

            s << keyValueSize;

            s.serialize(key.str(), keySize);
            s.serialize(value.str(), valueSize);
        }
    }
    else {
        // @@ Read key value pairs.
    }

    return s;
}




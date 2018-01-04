LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES+=$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/include     \
	$(LOCAL_PATH)/zbar \

ZXING:=0

ifeq ($(ZXING), 1)

LOCAL_C_INCLUDES+=$(LOCAL_PATH)/../stlport/stlport \
	$(LOCAL_PATH)/core/src/win32/zxing\
	$(LOCAL_PATH)/core/src\

LOCAL_SRC_FILES+= \
core/src/zxing/DecodeHints.cpp                                               \
core/src/zxing/pdf417/PDF417Reader.cpp                                       \
core/src/zxing/pdf417/decoder/ec/ModulusGF.cpp                               \
core/src/zxing/pdf417/decoder/ec/ErrorCorrection.cpp                         \
core/src/zxing/pdf417/decoder/ec/ModulusPoly.cpp                             \
core/src/zxing/pdf417/decoder/Decoder.cpp                                    \
core/src/zxing/pdf417/decoder/BitMatrixParser.cpp                            \
core/src/zxing/pdf417/decoder/DecodedBitStreamParser.cpp                     \
core/src/zxing/pdf417/detector/LinesSampler.cpp                              \
core/src/zxing/pdf417/detector/Detector.cpp                                  \
core/src/zxing/MultiFormatReader.cpp                                         \
core/src/zxing/LuminanceSource.cpp                                           \
core/src/zxing/ResultPoint.cpp                                               \
core/src/zxing/Exception.cpp                                                 \
core/src/zxing/common/GlobalHistogramBinarizer.cpp                           \
core/src/zxing/common/BitArray.cpp                                           \
core/src/zxing/common/CharacterSetECI.cpp                                    \
core/src/zxing/common/BitArrayIO.cpp                                         \
core/src/zxing/common/BitMatrix.cpp                                          \
core/src/zxing/common/BitSource.cpp                                          \
core/src/zxing/common/StringUtils.cpp                                        \
core/src/zxing/common/DecoderResult.cpp                                      \
core/src/zxing/common/GreyscaleRotatedLuminanceSource.cpp                    \
core/src/zxing/common/PerspectiveTransform.cpp                               \
core/src/zxing/common/HybridBinarizer.cpp                                    \
core/src/zxing/common/DetectorResult.cpp                                     \
core/src/zxing/common/GreyscaleLuminanceSource.cpp                           \
core/src/zxing/common/reedsolomon/GenericGF.cpp                              \
core/src/zxing/common/reedsolomon/ReedSolomonDecoder.cpp                     \
core/src/zxing/common/reedsolomon/GenericGFPoly.cpp                          \
core/src/zxing/common/reedsolomon/ReedSolomonException.cpp                   \
core/src/zxing/common/Str.cpp                                                \
core/src/zxing/common/IllegalArgumentException.cpp                           \
core/src/zxing/common/detector/MonochromeRectangleDetector.cpp               \
core/src/zxing/common/detector/WhiteRectangleDetector.cpp                    \
core/src/zxing/common/GridSampler.cpp                                        \
core/src/zxing/ChecksumException.cpp                                         \
core/src/zxing/ResultIO.cpp                                                  \
core/src/zxing/Result.cpp                                                    \
core/src/zxing/BarcodeFormat.cpp                                             \
core/src/zxing/Binarizer.cpp                                                 \
core/src/zxing/BinaryBitmap.cpp                                              \
core/src/zxing/FormatException.cpp                                           \
core/src/zxing/Reader.cpp                                                    \
core/src/zxing/qrcode/FormatInformation.cpp                                  \
core/src/zxing/qrcode/QRCodeReader.cpp                                       \
core/src/zxing/qrcode/ErrorCorrectionLevel.cpp                               \
core/src/zxing/qrcode/Version.cpp                                            \
core/src/zxing/qrcode/decoder/DataBlock.cpp                                  \
core/src/zxing/qrcode/decoder/DataMask.cpp                                   \
core/src/zxing/qrcode/decoder/Mode.cpp                                       \
core/src/zxing/qrcode/decoder/Decoder.cpp                                    \
core/src/zxing/qrcode/decoder/BitMatrixParser.cpp                            \
core/src/zxing/qrcode/decoder/DecodedBitStreamParser.cpp                     \
core/src/zxing/qrcode/detector/FinderPattern.cpp                             \
core/src/zxing/qrcode/detector/AlignmentPattern.cpp                          \
core/src/zxing/qrcode/detector/FinderPatternInfo.cpp                         \
core/src/zxing/qrcode/detector/FinderPatternFinder.cpp                       \
core/src/zxing/qrcode/detector/Detector.cpp                                  \
core/src/zxing/qrcode/detector/AlignmentPatternFinder.cpp                    \
core/src/zxing/multi/ByQuadrantReader.cpp                                    \
core/src/zxing/multi/GenericMultipleBarcodeReader.cpp                        \
core/src/zxing/multi/qrcode/QRCodeMultiReader.cpp                            \
core/src/zxing/multi/qrcode/detector/MultiFinderPatternFinder.cpp            \
core/src/zxing/multi/qrcode/detector/MultiDetector.cpp                       \
core/src/zxing/multi/MultipleBarcodeReader.cpp                               \
core/src/zxing/oned/OneDResultPoint.cpp                                      \
core/src/zxing/oned/Code93Reader.cpp                                         \
core/src/zxing/oned/CodaBarReader.cpp                                        \
core/src/zxing/oned/EAN8Reader.cpp                                           \
core/src/zxing/oned/Code39Reader.cpp                                         \
core/src/zxing/oned/UPCAReader.cpp                                           \
core/src/zxing/oned/UPCEReader.cpp                                           \
core/src/zxing/oned/Code128Reader.cpp                                        \
core/src/zxing/oned/UPCEANReader.cpp                                         \
core/src/zxing/oned/EAN13Reader.cpp                                          \
core/src/zxing/oned/MultiFormatOneDReader.cpp                                \
core/src/zxing/oned/ITFReader.cpp                                            \
core/src/zxing/oned/MultiFormatUPCEANReader.cpp                              \
core/src/zxing/oned/OneDReader.cpp                                           \
core/src/zxing/InvertedLuminanceSource.cpp                                   \
core/src/zxing/datamatrix/DataMatrixReader.cpp                               \
core/src/zxing/datamatrix/Version.cpp                                        \
core/src/zxing/datamatrix/decoder/DataBlock.cpp                              \
core/src/zxing/datamatrix/decoder/Decoder.cpp                                \
core/src/zxing/datamatrix/decoder/BitMatrixParser.cpp                        \
core/src/zxing/datamatrix/decoder/DecodedBitStreamParser.cpp                 \
core/src/zxing/datamatrix/detector/CornerPoint.cpp                           \
core/src/zxing/datamatrix/detector/DetectorException.cpp                     \
core/src/zxing/datamatrix/detector/Detector.cpp                              \
core/src/zxing/aztec/AztecReader.cpp                                         \
core/src/zxing/aztec/decoder/Decoder.cpp                                     \
core/src/zxing/aztec/detector/Detector.cpp                                   \
core/src/zxing/aztec/AztecDetectorResult.cpp                                 \
core/src/zxing/ResultPointCallback.cpp                                       \
core/src/bigint/BigIntegerAlgorithms.cpp                                      \
core/src/bigint/BigUnsignedInABase.cpp                                        \
core/src/bigint/BigInteger.cpp                                                \
core/src/bigint/BigIntegerUtils.cpp                                           \
core/src/bigint/BigUnsigned.cpp                                               \


LOCAL_LDFLAGS+=$(LOCAL_PATH)/../../prebuilts/ndk/7/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/libsupc++.a

LOCAL_LDFLAGS+=$(LOCAL_PATH)/../../prebuilts/ndk/8/platforms/android-14/arch-arm/usr/lib/libstdc++.a

LOCAL_SHARED_LIBRARIES += libstlport \

endif

LOCAL_SRC_FILES+= \
zbar/window.c                                                 \
zbar/processor/lock.c                                   \
zbar/processor/null.c                                   \
zbar/processor/posix.c                                  \
zbar/qrcode/qrdec.c                                     \
zbar/qrcode/util.c                                      \
zbar/qrcode/isaac.c                                     \
zbar/qrcode/rs.c                                        \
zbar/qrcode/binarize.c                                  \
zbar/qrcode/bch15_5.c                                   \
zbar/config.c                                                 \
zbar/video/null.c                                       \
zbar/scanner.c                                                \
zbar/decoder/ean.c                                      \
zbar/decoder/code128.c                                  \
zbar/decoder/qr_finder.c                                \
zbar/decoder/code39.c                                   \
zbar/decoder/i25.c                                      \
zbar/processor.c                                              \
zbar/symbol.c                                                 \
zbar/window/null.c                                            \
zbar/error.c                                                  \
zbar/video.c                                                  \
zbar/image.c                                                  \
zbar/convert.c                                                \
zbar/refcnt.c                                                 \
zbar/decoder.c                                                \
zbar/img_scanner.c                                            \
zbar/qrcode/qrdectxt.c                                  \
zbar/zbardec.c                                                            \


LOCAL_CFLAGS+= -Wno-error=non-virtual-dtor -fexceptions -DNO_ICONV

LOCAL_LDFLAGS+=  -lm  -ldl

LOCAL_LDLIBS+=-lpthread

LOCAL_LDFLAGS+=-lz -lm
	
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:=libqrdecode

LOCAL_SHARED_LIBRARIES += \
    libcutils       \
    libutils   \
	

LOCAL_STATIC_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES+=$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/core/lib/win32\
	$(LOCAL_PATH)/core/src\
	$(LOCAL_PATH)/../stlport/stlport \
	$(LOCAL_PATH)/include     \
	$(LOCAL_PATH)/zbar \

LOCAL_SRC_FILES+= \
cli/src/lodepng.cpp                                                          \
cli/src/main.cpp                                                             \
cli/src/jpgd.cpp                                                             \
cli/src/ImageReaderSource.cpp                                                \
cli/src/BufferBitmapSource.cpp\
cli/src/mybmp.c

LOCAL_CFLAGS+=-MD -MP -fPIC -DPIC -fexceptions -DNO_ICONV
LOCAL_LDFLAGS+=  -lm  -ldl

LOCAL_LDFLAGS+=$(LOCAL_PATH)/../../prebuilts/ndk/7/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/libsupc++.a

LOCAL_LDFLAGS+=$(LOCAL_PATH)/../../prebuilts/ndk/8/platforms/android-14/arch-arm/usr/lib/libstdc++.a

LOCAL_LDFLAGS+=-lz -lm -std=c99 -lc
	
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:=test_zxing

LOCAL_SHARED_LIBRARIES := \
    libcutils       \
    libutils   \
	libstlport


LOCAL_SHARED_LIBRARIES+=\
	libqrdecode \


include $(BUILD_EXECUTABLE)

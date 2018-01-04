#!/bin/sh

SRCDIR0=${OUT}/obj/STATIC_LIBRARIES

echo "--------------------------------------------------"
echo collect dir is: ${SRCDIR0}
echo "--------------------------------------------------"

cp $SRCDIR0/libdemux_asf_intermediates/libdemux_asf.a   ./
cp $SRCDIR0/libdemux_avi_intermediates/libdemux_avi.a   ./
cp $SRCDIR0/libdemux_ts_intermediates/libdemux_ts.a     ./
cp $SRCDIR0/libdemux_mkv_intermediates/libdemux_mkv.a   ./
cp $SRCDIR0/libdemux_ogg_intermediates/libdemux_ogg.a   ./
cp $SRCDIR0/libdemux_mov_intermediates/libdemux_mov.a   ./
cp $SRCDIR0/libdemux_flv_intermediates/libdemux_flv.a   ./
cp $SRCDIR0/libdemux_mpg_intermediates/libdemux_mpg.a   ./
cp $SRCDIR0/libdemux_pmp_intermediates/libdemux_pmp.a ./
cp $SRCDIR0/libdemux_raw_intermediates/libdemux_raw.a ./
#cp $SRCDIR0/libdemux_idxsub_intermediates/libdemux_idxsub.a ./
cp $SRCDIR0/libdemux_awts_intermediates/libdemux_awts.a ./
cp $SRCDIR0/libcedarxdemuxers_intermediates/libcedarxdemuxers.a ./
cp $SRCDIR0/libcedarxsftdemux_intermediates/libcedarxsftdemux.a ./
cp $SRCDIR0/libcedarxwvmdemux_intermediates/libcedarxwvmdemux.a ./
#cp $SRCDIR0/libcedarxstream_intermediates/libcedarxstream.a ./
cp $SRCDIR0/libuserdemux_intermediates/libuserdemux.a ./
cp $SRCDIR0/libcedarxrender_intermediates/libcedarxrender.a ./
cp $SRCDIR0/libcedarxcomponents_intermediates/libcedarxcomponents.a ./
#cp $SRCDIR0/libcedarxalloc_intermediates/libcedarxalloc.a ./
cp $SRCDIR0/libcedarxplayer_intermediates/libcedarxplayer.a ./
#cp $SRCDIR0/libjpgenc_intermediates/libjpgenc.a             ./
cp $SRCDIR0/libsub_intermediates/libsub.a             ./
#cp $SRCDIR0/libsub_inline_intermediates/libsub_inline.a             ./
cp $SRCDIR0/libmuxers_intermediates/libmuxers.a ./
cp $SRCDIR0/libmp4_muxer_intermediates/libmp4_muxer.a ./
cp $SRCDIR0/libmp3_muxer_intermediates/libmp3_muxer.a ./
cp $SRCDIR0/libaac_muxer_intermediates/libaac_muxer.a ./
cp $SRCDIR0/libawts_muxer_intermediates/libawts_muxer.a ./
cp $SRCDIR0/libraw_muxer_intermediates/libraw_muxer.a ./
cp $SRCDIR0/libmpeg2ts_muxer_intermediates/libmpeg2ts_muxer.a ./
cp $SRCDIR0/libm3u_intermediates/libm3u.a ./
cp $SRCDIR0/libcedara_decoder_intermediates/libcedara_decoder.a ./
cp $SRCDIR0/libcedarx_rtsp_intermediates/libcedarx_rtsp.a ./
cp $SRCDIR0/libcedarxdrmsniffer_intermediates/libcedarxdrmsniffer.a ./

cp $SRCDIR0/libstagefright_httplive_opt_intermediates/libstagefright_httplive_opt.a ./

cp $SRCDIR0/../../system/lib/libstagefright_soft_cedar_h264dec.so ./

#cp $SRCDIR0/../../system/lib/libcedarxosal.so ./
cp $SRCDIR0/../../system/lib/libcedarxbase.so ./
#cp $SRCDIR0/../../system/lib/libcedarxsftdemux.so ./
#cp $SRCDIR0/../../system/lib/libcedarv.so ./
#cp $SRCDIR0/../../system/lib/libswa1.so ./
cp $SRCDIR0/../../system/lib/libaw_audio.so ./
#cp $SRCDIR0/../../system/lib/libswa2.so ./
cp $SRCDIR0/../../system/lib/libaw_audioa.so ./
cp $SRCDIR0/../../system/lib/libswdrm.so ./
#cp $SRCDIR0/../../system/lib/libthirdpartstream.so ./
cp $SRCDIR0/../../system/lib/libcedarxsftstream.so ./
#cp $SRCDIR0/../../system/lib/libcedarv_base.so ./
#cp $SRCDIR0/../../system/lib/libcedarv_adapter.so ./
#cp $SRCDIR0/../../system/lib/libve.so ./

cp $SRCDIR0/../../system/lib/libfacedetection.so ./
#cp $SRCDIR0/../../system/lib/libsunxi_alloc.so ./
#cp $SRCDIR0/../../system/lib/libion_alloc.so ./
#cp $SRCDIR0/../../system/lib/libjpgenc.so ./
#cp $SRCDIR0/../../system/lib/libaw_h264enc.so ./

cp $SRCDIR0/../../system/lib/libVE.so ./
cp $SRCDIR0/../../system/lib/libvdecoder.so ./
cp $SRCDIR0/../../system/lib/librv.so ./
cp $SRCDIR0/../../system/lib/libvencoder.so ./
cp $SRCDIR0/../../system/lib/libMemAdapter.so ./

arm-linux-androideabi-strip -g *.a

cp ${OUT}/symbols/system/lib/libCedarX.so ~/tmp
current_time=`date +%Y%m%d%H%M`
mv ~/tmp/libCedarX.so ~/tmp/libCedarX-symbols-$current_time.so


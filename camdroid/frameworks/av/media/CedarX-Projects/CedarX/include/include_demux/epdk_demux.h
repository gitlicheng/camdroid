#ifndef CEDAR_DEMUX_H_
#define CEDAR_DEMUX_H_

#include "epdk_common.h"
#include "cedarx_demux.h"
#include "CDX_Debug.h"
#include "CDX_Common.h"

typedef struct EPDKDemuxerAPI
{
	const char *name;

	int (*open)(void **pParser, int nFmtType, CedarXDataSourceDesc *datasrc_desc);
	int (*close)(void **pParser);
	int (*control)(void *pParser, int cmd, int cmd_sub, void *para);
	int (*prefetch)(void *pParser, CedarXPacket *cdx_pkt);
	int (*read)(void *pParser, CedarXPacket *cdx_pkt);
	int (*read_dummy)(void *pParser, CedarXPacket *cdx_pkt);
	int (*getmediainfo)(void *pParser, CedarXMediainfo *pMediaInfo);
}EPDKDemuxerAPI;

extern CedarXDemuxerAPI cdx_dmxs_epdk;

//cedarx demuxers
extern EPDKDemuxerAPI epdk_demux_mov;
extern EPDKDemuxerAPI epdk_demux_mkv;
extern EPDKDemuxerAPI epdk_demux_asf;
extern EPDKDemuxerAPI epdk_demux_mpg;
extern EPDKDemuxerAPI epdk_demux_ts;
extern EPDKDemuxerAPI epdk_demux_avi;
extern EPDKDemuxerAPI epdk_demux_flv;
extern EPDKDemuxerAPI epdk_demux_pmp;
extern EPDKDemuxerAPI epdk_demux_ogg;

#endif /* CEDAR_DEMUX_H_ */

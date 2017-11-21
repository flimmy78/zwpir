/**********************  ZWZip6lowPanIphc.h  *******************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2009
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description:
 *
 * Author:      Valeriy Vyshnyak
 *
 * Last Changed By:  $Author: vvi $
 * Revision:         $Revision: 0000 $
 * Last Changed:     $Date: Sep 25, 2009 3:36:57 AM $
 *
 ****************************************************************************/

#ifndef ZWZIP6LOWPANIPHC_H_
#define ZWZIP6LOWPANIPHC_H_

///#include "types.h"
#include "ZW_typedefs.h"



#define LOWPAN_CONVERTER_IPV6_PACKET_SIZE_MAX      2048

#ifdef __cplusplus
extern "C"
{
#endif

extern void LowpanIphcToIpv6(const BYTE* inLowpanData, WORD inLowPanDataSize, BYTE* outIpv6Data, WORD* outIpv6DataSize, const BYTE* homeIdBigEndian, BYTE sourceNodeId, BYTE destinationNodeId);
extern void LowpanIpv6ToIphc(const BYTE* inIpv6Data, WORD inIpv6DataSize, BYTE* outLowpanData, WORD* outLowPanDataSize, const BYTE* homeIdBigEndian, BYTE sourceNodeId, BYTE destinationNodeId);

#ifdef __cplusplus
}
#endif

#endif /* ZWZIP6LOWPANIPHC_H_ */

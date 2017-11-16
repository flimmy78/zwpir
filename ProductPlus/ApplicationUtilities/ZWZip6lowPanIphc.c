/**********************  ZWZip6lowPanIphc.c  *******************************
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

#include "ZWZip6lowPanIphc.h"
#include "ZW_typedefs.h"
#include "ZW_classcmd.h"
#include "ZW_classcmd_ex.h"

// IPv6 <-> 6lowPAN IPHC converters: begin code
void LowpanIphcToIpv6(
  const BYTE* inLowpanData, WORD inLowPanDataSize,
  BYTE* outIpv6Data, WORD* outIpv6DataSize,
  const BYTE* homeIdBigEndian, BYTE sourceNodeId, BYTE destinationNodeId)
{
  BYTE header1 = inLowpanData[0];
  BYTE header2 = inLowpanData[1];
  WORD outIpv6DataLenMax = *outIpv6DataSize;
  WORD lowpan_idx = LOWPAN_IPHC_6LOWPAN_HEADER_SIZE_MIN;
  WORD ipv6_payload_lengh;
  BYTE prev_next_header_field_idx = 0;
  BYTE i, n;
  BYTE is_next_header_compressed;

  *outIpv6DataSize = 0;
  if((header1 & 0xe0) != 0x60             //LOWPAN_IPHC id
    || inLowPanDataSize < LOWPAN_IPHC_6LOWPAN_HEADER_SIZE_MIN
    || outIpv6DataLenMax < 40)             //IPv6 Header size

  {
    return;
  }

  //CID: Context Identifier Extension:
  if(header2 & LOWPAN_IPHC_HEADER_2_CID_BIT_MASK)
  {
    //(NOT IMPLEMENTED) CID BYTE ignored
    lowpan_idx++;
  }

  //IPv6 Header Version:
  outIpv6Data[0] = 0x60;

  //IPv6 Traffic Class, Flow Label:
  //TF: Traffic Class, Flow Label:
  switch ((header1 & LOWPAN_IPHC_HEADER1_TF_MASK) >> LOWPAN_IPHC_HEADER1_TF_SHIFT)
  {
  case 0:                 //00:  4-bit Pad + Traffic Class + Flow Label (4 BYTEs)
    i = ((inLowpanData[lowpan_idx] & 0xC0) >> 6) | ((inLowpanData[lowpan_idx] & 0x3f) << 2); // get the rotated right by 2 bits Traffic class
    lowpan_idx++;
    outIpv6Data[0] |= (i >> 4) & 0x0f;
    outIpv6Data[1] = ((i << 4) & 0xf0) | (inLowpanData[lowpan_idx++] & 0x0f);
    outIpv6Data[2] = inLowpanData[lowpan_idx++];
    outIpv6Data[3] = inLowpanData[lowpan_idx++];
    break;
  case 1:                 //01:  ECN + 2-bit Pad + Flow Label (3 BYTEs)
    i = ((inLowpanData[lowpan_idx] & 0xC0) >> 6); // get the rotated right by 2 bits Traffic class
    outIpv6Data[1] = ((i << 4) & 0xf0) | (inLowpanData[lowpan_idx] & 0x0f);
    lowpan_idx++;
    outIpv6Data[2] = inLowpanData[lowpan_idx++];
    outIpv6Data[3] = inLowpanData[lowpan_idx++];
    break;
  case 2:                 //10:  Traffic Class (1 BYTE)
    i = ((inLowpanData[lowpan_idx] & 0xC0) >> 6) | ((inLowpanData[lowpan_idx] & 0x3f) << 2); // get the rotated right by 2 bits Traffic class
    lowpan_idx++;
    outIpv6Data[0] |= (i >> 4) & 0x0f;
    outIpv6Data[1] = ((i << 4) & 0xf0);
    outIpv6Data[2] = 0;
    outIpv6Data[3] = 0;
    break;
  case 3:                 //11:  Version, Traffic Class, and Flow Label are compressed.
    outIpv6Data[1] = 0;
    outIpv6Data[2] = 0;
    outIpv6Data[3] = 0;
    break;
  }

  //Restore the IPv6 Next Header
  //NH: Next Header:
  if(header1 & LOWPAN_IPHC_HEADER1_NH_BIT_MASK)
  {
    //   1: The Next Header field is compressed and the next header is
    //      compressed using LOWPAN_NHC, which is discussed in Section 3.
    is_next_header_compressed = 1;
    prev_next_header_field_idx = 6;              //Offset of the IPv6 Header Next Header field;
  }
  else
  {
    //   0: Full 8 bits for Next Header are carried in-line.
    outIpv6Data[6] = inLowpanData[lowpan_idx++];    //IPv6 Header Next Header
    is_next_header_compressed = 0;
    prev_next_header_field_idx = 0;
  }

  //Restore the IPv6 HopLimit:
  //HLIM: Hop Limit:
  //   00:  The Hop Limit field is carried in-line.
  //   01:  The Hop Limit field is elided and the the hop limit is 1.
  //   10:  The Hop Limit field is elided and the the hop limit is 64.
  //   11:  The Hop Limit field is elided and the hop limit is 255.
  switch ((header1 & LOWPAN_IPHC_HEADER1_HLIM_MASK) >> 0)
  {
  case 0:                 //00: The Hop Limit field is carried in-line.
    outIpv6Data[7] = inLowpanData[lowpan_idx++];
    break;
  case 1:                 //01: The Hop Limit field is elided and the the hop limit is 1.
    outIpv6Data[7] = 1;
    break;
  case 2:                 //10: The Hop Limit field is elided and the the hop limit is 64.
    outIpv6Data[7] = 64;
    break;
  case 3:                 //11: The Hop Limit field is elided and the hop limit is 255.
    outIpv6Data[7] = 255;
    break;
  }

  //Restore the IPv6 Source Address
  //SAC: Source Address Compression
  if (header2 & LOWPAN_IPHC_HEADER_2_SAC_BIT_MASK)
  {
    //      1: Source address compression uses stateful, context-based
    //         compression.
    // (NOT IMPLEMENTED)
  }
  else
  {
    //      0: Source address compression uses stateless compression.
    //SAM: Source Address Mode:
    i = (header2 & LOWPAN_IPHC_HEADER_2_SAM_MASK) >> LOWPAN_IPHC_HEADER_2_SAM_SHIFT;
    switch (i)
    {
    case 0x00:
      //      00:  128 bits.  The full address is carried in-line.
      for (i = 0; i < 16; i++)
      {
        outIpv6Data[8 + i] =  inLowpanData[lowpan_idx++];
      }
      break;

    case 0x01:
      //      01:  N/A
      break;

    case 0x03:
    case 0x02:
      //      10:  16 bits.  The first 112 bits of the address are elided.
      //         The value of those bits is the link-local prefix padded with
      //         zeros.  The remaining 16 bits are carried inline.
      //         Z-Wave mesh-under: N/A
      //         Z-Wave route-over: 16 bits inline
      outIpv6Data[ 8] = 0xfe;
      outIpv6Data[ 9] = 0x80;
      outIpv6Data[10] = 0x00;
      outIpv6Data[11] = 0x00;
      outIpv6Data[12] = 0x00;
      outIpv6Data[13] = 0x00;
      outIpv6Data[14] = 0x00;
      outIpv6Data[15] = 0x00;
      outIpv6Data[16] = 0x02;
      outIpv6Data[17] = 0x1e;
      outIpv6Data[18] = 0x32;
      outIpv6Data[19] = homeIdBigEndian[0];
      outIpv6Data[20] = homeIdBigEndian[1];
      outIpv6Data[21] = homeIdBigEndian[2];

      if(i == 0x03)
      {
        //      11:  0 bits.  The address is fully elided.  The first 64 bits
        //         of the address are the link-local prefix padded with zeros.
        //         The remaining 64 bits are computed from the link-layer
        //         address as 0x021E32 + HomeID + NodeID.
        outIpv6Data[22] = homeIdBigEndian[3];
        outIpv6Data[23] = sourceNodeId;
      }
      else
      {
        outIpv6Data[22] = inLowpanData[lowpan_idx++];
        outIpv6Data[23] = inLowpanData[lowpan_idx++];
      }
      break;
    }
  }

  //Restore the IPv6 Destination Address
  //M: Multicast Compression
  if (header2 & LOWPAN_IPHC_HEADER_2_SAC_BIT_MASK)
  {
    //   1: Destination address uses multicast compression.
    //DAC: Destination Address Compression
    if (header2 & LOWPAN_IPHC_HEADER_2_DAC_BIT_MASK)
    {
      //   1: Destination address compression uses stateful, context-based
      //      compression.
      // (NOT IMPLEMENTED)
    }
    else
    {
      //   0: Destination address compression uses stateless compression.
      outIpv6Data[24] = 0xff;
      outIpv6Data[26] = 0x00;
      outIpv6Data[27] = 0x00;
      outIpv6Data[28] = 0x00;
      outIpv6Data[29] = 0x00;
      outIpv6Data[30] = 0x00;
      outIpv6Data[31] = 0x00;
      outIpv6Data[32] = 0x00;
      outIpv6Data[33] = 0x00;
      outIpv6Data[34] = 0x00;
      //DAM: Destination Address Mode:
      switch ((header2 & LOWPAN_IPHC_HEADER_2_DAM_MASK) >> 0)
      {
      case 0x00:
        //00:  48 bits.  The address takes the form FFXX::00XX:XXXX:XXXX.
        outIpv6Data[25] = inLowpanData[lowpan_idx++];
        outIpv6Data[35] = inLowpanData[lowpan_idx++];
        outIpv6Data[36] = inLowpanData[lowpan_idx++];
        outIpv6Data[37] = inLowpanData[lowpan_idx++];
        outIpv6Data[38] = inLowpanData[lowpan_idx++];
        outIpv6Data[39] = inLowpanData[lowpan_idx++];
        break;

      case 0x01:
        //01:  32 bits.  The address takes the form FFXX::00XX:XXXX.
        outIpv6Data[25] = inLowpanData[lowpan_idx++];
        outIpv6Data[35] = 0x00;
        outIpv6Data[36] = 0x00;
        outIpv6Data[37] = inLowpanData[lowpan_idx++];
        outIpv6Data[38] = inLowpanData[lowpan_idx++];
        outIpv6Data[39] = inLowpanData[lowpan_idx++];
        break;

      case 0x02:
        //10:  16 bits.  The address takes the form FF0X::0XXX.
        i = inLowpanData[lowpan_idx++];
        outIpv6Data[25] = (i >> 4) & 0x0f;
        outIpv6Data[35] = 0x00;
        outIpv6Data[36] = 0x00;
        outIpv6Data[37] = 0x00;
        outIpv6Data[38] = (i >> 0) & 0x0f;
        outIpv6Data[39] = inLowpanData[lowpan_idx++];
        break;

      case 0x03:
        //11:  8 bits.  The address takes the form FF02::00XX.
        outIpv6Data[25] = 0x02;
        outIpv6Data[35] = 0x00;
        outIpv6Data[36] = 0x00;
        outIpv6Data[37] = 0x00;
        outIpv6Data[38] = 0x00;
        outIpv6Data[39] = inLowpanData[lowpan_idx++];
        break;
      }
    }
  }
  else
  {
    //   0: Destination address does not use multicast compression.
    //DAC: Destination Address Compression
    if (header2 & LOWPAN_IPHC_HEADER_2_DAC_BIT_MASK)
    {
      //   1: Destination address compression uses stateful, context-based
      //      compression.
      // (NOT IMPLEMENTED)
    }
    else
    {
      //   0: Destination address compression uses stateless compression.
      //DAM: Destination Address Mode:
      i = (header2 & LOWPAN_IPHC_HEADER_2_DAM_MASK) >> 0;
      switch (i)
      {
      case 0x00:
        //00:  128 bits.  The full address is carried in-line.
        for (i = 0; i < 16; i++)
        {
          outIpv6Data[24 + i] =  inLowpanData[lowpan_idx++];
        }
        break;

      case 0x01:
        //01:  N/A
        break;

      case 0x03:
      case 0x02:
        //10:  16 bits.  The first 112 bits of the address are elided.
        //      The value of those bits is the link-local prefix padded
        //      with zeros.  The remaining 16 bits are carried inline.
        //      Z-Wave mesh-under: N/A
        //      Z-Wave route-over: 16 bits inline
        outIpv6Data[24] = 0xfe;
        outIpv6Data[25] = 0x80;
        outIpv6Data[26] = 0x00;
        outIpv6Data[27] = 0x00;
        outIpv6Data[28] = 0x00;
        outIpv6Data[29] = 0x00;
        outIpv6Data[30] = 0x00;
        outIpv6Data[31] = 0x00;
        outIpv6Data[32] = 0x02;
        outIpv6Data[33] = 0x1e;
        outIpv6Data[34] = 0x32;
        outIpv6Data[35] = homeIdBigEndian[0];
        outIpv6Data[36] = homeIdBigEndian[1];
        outIpv6Data[37] = homeIdBigEndian[2];

        if(i == 0x03)
        {
          //11:  0 bits.  The address is fully elided.  The first 64
          //      bits of the address are the link-local prefix padded with
          //      zeros.  The remaining 64 bits are computed from the link-
          //      layer address as 0x021E32 + HomeID + NodeID.
          outIpv6Data[38] = homeIdBigEndian[3];
          outIpv6Data[39] = destinationNodeId;
        }
        else
        {
          outIpv6Data[38] = inLowpanData[lowpan_idx++];
          outIpv6Data[39] = inLowpanData[lowpan_idx++];
        }
        break;
      }
    }
  }

  //Restore the IPv6 Extension Headers:
  ipv6_payload_lengh = 0;
  // Compressed headers handling:
  while(is_next_header_compressed)
  {
    i = inLowpanData[lowpan_idx++];

    if ((i & 0xf0) == 0xE0)
    {
      // LOWPAN_NHC IPv6 Extension Header Compression

      //NH: Next Header:
      //   0: Full 8 bits for Next Header are carried in-line.
      //   1: The Next Header field is compressed and the next header is
      //      compressed using LOWPAN_NHC, which is discussed in Section 3.
      is_next_header_compressed = i & 0x01;

      //EID: IPv6 Extension Header ID:
      i = (i >> 1) & 0x07;
      switch (i)
      {
      case 0:
        //   0: IPv6 Hop-by-Hop Options [RFC2460]
      case 1:
        //   1: IPv6 Routing [RFC2460]
      case 3:
        //   3: IPv6 Destination Options [RFC2460]
      case 4:
        //   4: IPv6 Mobility Header [RFC3775]

        //Restoring the IPv6 Extension Header Next Header Field:
        //The Next Header Field contained in IPv6 Extension Headers is elided
        // when the NH bit is set in the LOWPAN_NHC encoding octet.  Note that
        // doing so allows LOWPAN_NHC to utilize no more overhead than the non-
        // encoded IPv6 Extension Header.
        switch (i)
        {
        case 0:
          //   0: IPv6 Hop-by-Hop Options [RFC2460]
          outIpv6Data[prev_next_header_field_idx] = 0;        //IPv6 Hop-by-Hop Options [RFC2460]
          break;
        case 1:
          //   1: IPv6 Routing [RFC2460]
          outIpv6Data[prev_next_header_field_idx] = 43;       //IPv6 Routing [RFC2460]
          break;
        case 3:
          //   3: IPv6 Destination Options [RFC2460]
          outIpv6Data[prev_next_header_field_idx] = 60;       //IPv6 Destination Options [RFC2460]
          break;
        case 4:
          //   4: IPv6 Mobility Header [RFC3775]
          outIpv6Data[prev_next_header_field_idx] = 135;      //IPv6 Mobility Header [RFC3775]
          break;
        }
        prev_next_header_field_idx = 40 + ipv6_payload_lengh++;    //store the offset on the IPv6 Extension Header Next field.

        //Restoring the IPv6 Extension Header Header Length Field:
        //The Length Field contained in IPv6 Extension Headers indicate the
        //length of the IPv6 Extension Header in octets, not including the
        //LOWPAN_NHC BYTE.  Note that this changes the standard Length Field
        //definition from indicating the header size in 8-octet units, not
        //including the first 8 octets.  Changing the Length Field to be in
        //units of octets removes wasteful internal fragmentation.  However,
        //specifying units in octets also means that LOWPAN_NHC CANNOT be used
        //to encode IPv6 Extension Headers that exceed 255 octets.
        n = inLowpanData[lowpan_idx++];
        if (n < 7)
        {
          outIpv6Data[40 + ipv6_payload_lengh++] = 0;
        }
        else
        {
          outIpv6Data[40 + ipv6_payload_lengh++] = (n + 1) / 8 - 1;
        }

        //copy header data:
        //TODO: IPv6 Hop-by-Hop and Destination Options Headers may use Pad1 and PadN
        // to pad out the header to a multiple of 8 octets in length.  When
        // using LOWPAN_NHC, those Pad1 and PadN options MAY be elided and the
        // length of the header reduced by the size of those Pad1 and PadN
        // options.  When converting from the LOWPAN_NHC encoding back to the
        // standard IPv6 encoding, Pad1 and PadN options MUST be used to pad out
        // the containing header to a multiple of 8 octets in length if
        // necessary.  Note that Pad1 and PadN options that do not appear at the
        // end of the containing header MUST NOT be elided as they are used to
        // align subsequent options.
        for(i = 0; i < n - 1; i++)      //exclude (-1) the Length Field of the LOWPAN_NHC compressed header.
        {
          outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        }
        break;

      case 2:
        //   2: IPv6 Fragment [RFC2460]
        outIpv6Data[prev_next_header_field_idx] = 44;       //IPv6 Fragment [RFC2460]
        is_next_header_compressed = 0;                      //this header is last header, no more processing.
        prev_next_header_field_idx = 0;                     //next header field for this header is stored inline.

        //The rest data in the lowPAN buffer is a IPv6 Fragment header and data.
        while(lowpan_idx < inLowPanDataSize)
        {
          outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        }
        break;

      case 7:
        //   7: IPv6 Header
        outIpv6Data[prev_next_header_field_idx] = 41;       //IPv6 Header
        is_next_header_compressed = 0;                      //this header is last header, no more processing.
        prev_next_header_field_idx = 0;                     //next header field for this header is stored inline.

        //TODO: recursive IPv6 Header compression. When the identified next header is an IPv6 Header (EID=7), the NH bit
        // of the LOWPAN_NHC encoding is unused and SHOULD be set to zero.  The
        // bytes following follow the LOWPAN_IPHC encoding as defined in Section 2.
        break;

      default:
        //   5: Reserved
        //   6: Reserved
        is_next_header_compressed = 0;
        break;
      }
    }
    else if ((i & 0xf8) == 0xf0)
    {
      //LOWPAN_NHC UDP Header Compression
      outIpv6Data[prev_next_header_field_idx] = 17;       //UDP
      is_next_header_compressed = 0;                      //this header is last header, no more processing.
      prev_next_header_field_idx = 0;                     //next header field for this header is not exist.

      //Restoring the Source and destination ports of UDP Header of IPv6 Packet:
      //P: Ports:
      switch (i & 0x03)
      {
      case 0x00:
        //00:  All 16 bits for both Source Port and Destination Port are
        //   carried in-line.
        for (n = 0; n < 4; n++)
        {
          outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        }
        break;

      case 0x01:
        //01:  All 16 bits for Source Port are carried in-line.  First 8
        //   bits of Destination Port is 0xF0 and elided.  The remaining 8
        //   bits of Destination Port are carried in-line.
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = 0xf0;
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        break;

      case 0x02:
        //10:  First 8 bits of Source Port are 0xF0 and elided.  The
        //   remaining 8 bits of Source Port are carried in-line.  All 16
        //   bits for Destination Port are carried in-line.
        outIpv6Data[40 + ipv6_payload_lengh++] = 0xf0;
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        break;

      case 0x03:
        //11:  First 12 bits of both Source Port and Destination Port are
        //   0xF0B and elided.  The remaining 4 bits for each are carried
        //   in-line.
        n = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = 0xf0;
        outIpv6Data[40 + ipv6_payload_lengh++] = 0xb0 | ((n >> 4) & 0x0f);
        outIpv6Data[40 + ipv6_payload_lengh++] = 0xf0;
        outIpv6Data[40 + ipv6_payload_lengh++] = 0xb0 | ((n >> 0) & 0x0f);
        break;
      }

      //Restoring the Length field of UDP Header of IPv6 Packet:
      n = 40 + ipv6_payload_lengh;       // store the offset of lenth field. it will be updated below.
      ipv6_payload_lengh += 2;

      //Restoring the Checksum field of UDP Header of IPv6 Packet:
      //C: Checksum:
      if (i & 0x04)
      {
        //1: All 16 bits of Checksum are elided.  The Checksum is recovered
        //  by recomputing it on the 6LoWPAN termination point.
        //  ZW: N/A – unless 16 checksum is provided by all network nodes.
        // (NOT IMPLEMENTED)
        outIpv6Data[40 + ipv6_payload_lengh++] = 0x00;
        outIpv6Data[40 + ipv6_payload_lengh++] = 0x00;
      }
      else
      {
        //0: All 16 bits of Checksum are carried in-line.
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
      }

      //Restoring the payload data of the UDP packet:
      //The rest data in the lowPAN buffer is a UDP payload data.
      while(lowpan_idx < inLowPanDataSize)
      {
        outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
      }

      //Update the Length field of UDP Header of IPv6 Packet:
      outIpv6Data[n++] = (ipv6_payload_lengh >> 8) & 0xff;
      outIpv6Data[n++] = (ipv6_payload_lengh >> 0) & 0xff;
    }
    else
    {
      //error! Unknown LOWPAN header compression.
      is_next_header_compressed = 0;
    }
  }

  // Uncompressed headers handling:

  if (!is_next_header_compressed)
  {
    //The rest data in the lowPAN buffer is a uncompressed headers and its data.
    while(lowpan_idx < inLowPanDataSize)
    {
      outIpv6Data[40 + ipv6_payload_lengh++] = inLowpanData[lowpan_idx++];
    }
  }

  //Restore the IPv6 Payload Length:
  outIpv6Data[4] = (ipv6_payload_lengh >> 8) & 0xff;
  outIpv6Data[5] = (ipv6_payload_lengh >> 0) & 0xff;

  //Update Return variables:
  if(ipv6_payload_lengh + 40 > outIpv6DataLenMax)
  {
    //TODO: before each increment of ipv6_payload_lengh we should check, if it not greater than outIpv6DataLenMax!
  }
  *outIpv6DataSize = ipv6_payload_lengh + 40;   // 40- IPv6 Header size;
}

void LowpanIpv6ToIphc(
  const BYTE* inIpv6Data, WORD inIpv6DataSize,
  BYTE* outLowpanData, WORD* outLowPanDataSize,
  const BYTE* homeIdBigEndian, BYTE sourceNodeId, BYTE destinationNodeId)
{
#define header1 outLowpanData[0]
#define header2 outLowpanData[1]
#define payload outLowpanData
  WORD payloadLenMax = *outLowPanDataSize - LOWPAN_IPHC_6LOWPAN_HEADER_SIZE_MIN;

  WORD payloadIdx = LOWPAN_IPHC_6LOWPAN_HEADER_SIZE_MIN;
  BYTE i, n;
  WORD iw, nw;
  BYTE prev_hext_header_idx;
  BYTE next_header;
  WORD next_header_idx;
  BYTE traffic_class;

  header1 = LOWPAN_IPHC;
  header2 = 0;

  *outLowPanDataSize = 0;
  if((inIpv6Data[0] & 0xf0) != 0x60       //IPv6 Header id
    || inIpv6DataSize < 40)             //IPv6 Header size
  {
    return;
  }



  //TF: Traffic Class, Flow Label:
  /*BYTE*/ traffic_class = ((inIpv6Data[0] << 4 ) & 0xf0) | ((inIpv6Data[1] >> 4) & 0x0f);
  if (*((DWORD*)&inIpv6Data[0]) == 0x00000060)
  {
   //      11:  Version, Traffic Class, and Flow Label are compressed.
   header1 |= (0x03 << LOWPAN_IPHC_HEADER1_TF_SHIFT) & LOWPAN_IPHC_HEADER1_TF_MASK;
  }
  else if (*((WORD*)&inIpv6Data[2]) == 0 && (inIpv6Data[1] & 0x0f) == 0)
  {
    //      10:  Traffic Class (1 byte)
    header1 |= (0x02 << LOWPAN_IPHC_HEADER1_TF_SHIFT) & LOWPAN_IPHC_HEADER1_TF_MASK;
    payload[payloadIdx++] = ((traffic_class << 6) & 0xC0) | (((traffic_class >> 2) & 0x3f));
  }
  else if ((traffic_class & 0xFC) == 0)
  {
    //      01:  ECN + 2-bit Pad + Flow Label (3 bytes)
    header1 |= (0x01 << LOWPAN_IPHC_HEADER1_TF_SHIFT) & LOWPAN_IPHC_HEADER1_TF_MASK;
    payload[payloadIdx++] = ((traffic_class << 6) & 0xC0) | (inIpv6Data[1] & 0x0f);
    payload[payloadIdx++] = inIpv6Data[2];
    payload[payloadIdx++] = inIpv6Data[3];
  }
  else
  {
    //      00:  4-bit Pad + Traffic Class + Flow Label (4 bytes)
    payload[payloadIdx++] = ((traffic_class << 6) & 0xC0) | (((traffic_class >> 2) & 0x3f));
    payload[payloadIdx++] = inIpv6Data[1] & 0x0f;
    payload[payloadIdx++] = inIpv6Data[2];
    payload[payloadIdx++] = inIpv6Data[3];
  }



  //NH: Next Header:
  switch (inIpv6Data[6])    //IPv6 Header Next Header
  {
  case 0:       //IPv6 Hop-by-Hop Options [RFC2460]
  case 43:      //IPv6 Routing [RFC2460]
  case 44:      //IPv6 Fragment [RFC2460]
  case 60:      //IPv6 Destination Options [RFC2460]
  case 135:     //IPv6 Mobility Header [RFC3775]
  case 41:      //IPv6 Header
  case 17:      //UDP
    //   1: The Next Header field is compressed and the next header is
    //      compressed using LOWPAN_NHC, which is discussed in Section 3.
    header1 |= LOWPAN_IPHC_HEADER1_NH_BIT_MASK;
    break;
  default:
    //   0: Full 8 bits for Next Header are carried in-line.
    payload[payloadIdx++] = inIpv6Data[6];    //IPv6 Header Next Header
  }



  //HLIM: Hop Limit:
  switch (inIpv6Data[7])    //IPv6 Header Hop Limit
  {
  case 255:
    //      11:  The Hop Limit field is elided and the hop limit is 255.
    header1 |= (0x03 << 0) & LOWPAN_IPHC_HEADER1_HLIM_MASK;
    break;
  case 64:
    //      10:  The Hop Limit field is elided and the the hop limit is 64.
    header1 |= (0x02 << 0) & LOWPAN_IPHC_HEADER1_HLIM_MASK;
    break;
  case 1:
    //      01:  The Hop Limit field is elided and the the hop limit is 1.
    header1 |= (0x01 << 0) & LOWPAN_IPHC_HEADER1_HLIM_MASK;
    break;
  default:
    //      00:  The Hop Limit field is carried in-line.
    payload[payloadIdx++] = inIpv6Data[7];    //IPv6 Header Hop Limit
    break;
  }



  //CID: Context Identifier Extension:
  //   0: No additional 8-bit Context Identifier Extension is used.  If
  //      context-based compression is specified in either SC or DC,
  //     context 0 is used.
  //   1: An additional 8-bit Context Identifier Extension field
  //      immediately follows the DAM field.
  // (NOT IMPLEMENTED)



  //SAC: Source Address Compression
  //   0: Source address compression uses stateless compression.
  //   1: (NOT IMPLEMENTED). Source address compression uses stateful, context-based
  //      compression.
  //
  //SAM: Source Address Mode:
  //   If SAC=0:
  //      00:  128 bits.  The full address is carried in-line.
  //      01:  N/A
  //      10:  16 bits.  The first 112 bits of the address are elided.
  //         The value of those bits is the link-local prefix padded with
  //         zeros.  The remaining 16 bits are carried inline.
  //         Z-Wave mesh-under: N/A
  //         Z-Wave route-over: 16 bits inline
  //
  //      11:  0 bits.  The address is fully elided.  The first 64 bits
  //         of the address are the link-local prefix padded with zeros.
  //         The remaining 64 bits are computed from the link-layer
  //         address as 0x021E32 + HomeID + NodeID.
  //   If SAC=1: (NOT IMPLEMENTDED)
  //      Note: Do not implement now but prepare software structures for
  //      using context ID lookup in the future.
  //      00:  Reserved.
  //      01:  N/A
  //      10:  16 bits.  The address is derived using context information
  //         and the 16 bits carried inline.
  //      11:  0 bits.  The address is derived using context information
  //         and possibly link-layer addresses.
  if(inIpv6Data[8] == 0xfe && inIpv6Data[9] == 0x80 && inIpv6Data[10] == 0x00 && inIpv6Data[11] == 0x00
      && inIpv6Data[12] == 0x00 && inIpv6Data[13] == 0x00 && inIpv6Data[14] == 0x00 && inIpv6Data[15] == 0x00
      && inIpv6Data[16] == 0x02 && inIpv6Data[17] == 0x1e && inIpv6Data[18] == 0x32 && inIpv6Data[19] == homeIdBigEndian[0]
      && inIpv6Data[20] == homeIdBigEndian[1] && inIpv6Data[21] == homeIdBigEndian[2])
  {
    if(inIpv6Data[22] == homeIdBigEndian[3] && inIpv6Data[23] == sourceNodeId)
    {
      //   0: Source address compression uses stateless compression.
      //      11:  0 bits.  The address is fully elided.  The first 64 bits
      //         of the address are the link-local prefix padded with zeros.
      //         The remaining 64 bits are computed from the link-layer
      //         address as 0x021E32 + HomeID + NodeID.
      header2 |= (0x03 << LOWPAN_IPHC_HEADER_2_SAM_SHIFT) & LOWPAN_IPHC_HEADER_2_SAM_MASK;
    }
    else
    {
      //   0: Source address compression uses stateless compression.
      //      10:  16 bits.  The first 112 bits of the address are elided.
      //         The value of those bits is the link-local prefix padded with
      //         zeros.  The remaining 16 bits are carried inline.
      //         Z-Wave mesh-under: N/A
      //         Z-Wave route-over: 16 bits inline
      header2 |= (0x02 << LOWPAN_IPHC_HEADER_2_SAM_SHIFT) & LOWPAN_IPHC_HEADER_2_SAM_MASK;
      payload[payloadIdx++] = homeIdBigEndian[3];
      payload[payloadIdx++] = sourceNodeId;
    }
  }
  else
  {
    //   0: Source address compression uses stateless compression.
    //      00:  128 bits.  The full address is carried in-line.
    for(i = 0; i < 16; i++)
    {
      payload[payloadIdx++] = inIpv6Data[8 + i];    //IPv6 Header Source Address
    }
  }



  //M: Multicast Compression
  //      0: Destination address does not use multicast compression.
  //      1: Destination address uses multicast compression.
  //
  //   DAC: Destination Address Compression
  //      0: Destination address compression uses stateless compression.
  //      1: (NOT IMPLEMENTED) Destination address compression uses stateful, context-based
  //         compression.
  //
  //   DAM: Destination Address Mode:
  //      If M=0:
  //         If DAC=0:
  //            00:  128 bits.  The full address is carried in-line.
  //            01:  N/A
  //            10:  16 bits.  The first 112 bits of the address are elided.
  //               The value of those bits is the link-local prefix padded
  //               with zeros.  The remaining 16 bits are carried inline.
  //               Z-Wave mesh-under: N/A
  //               Z-Wave route-over: 16 bits inline
  //
  //            11:  0 bits.  The address is fully elided.  The first 64
  //               bits of the address are the link-local prefix padded with
  //               zeros.  The remaining 64 bits are computed from the link-
  //               layer address as 0x021E32 + HomeID + NodeID.
  //
  //         If DAC=1: (NOT IMPLEMENTED)
  //            Note: Do not implement now but prepare software structures for
  //            using context ID lookup in the future.
  //            00:  Reserved.
  //            01:  N/A
  //            10:  16 bits.  The address is derived using context
  //               information and the 16 bits carried inline.
  //            11:  0 bits.  The address is derived using context
  //               information and possibly link-layer addresses.
  //      If M=1 and DAC=0:
  //         00:  48 bits.  The address takes the form FFXX::00XX:XXXX:XXXX.  FFXX:0000:0000:0000:0000:00XX:XXXX:XXXX
  //         01:  32 bits.  The address takes the form FFXX::00XX:XXXX.       FFXX:0000:0000:0000:0000:0000:00XX:XXXX
  //         10:  16 bits.  The address takes the form FF0X::0XXX.            FF0X:0000:0000:0000:0000:0000:0000:0XXX
  //         11:  8 bits.  The address takes the form FF02::00XX.             FF02:0000:0000:0000:0000:0000:0000:00XX
  //      If M=1 and DAC=1: (NOT IMPLEMENTED)
  //         00:  128 bits.  The full address is carried in-line.
  //         01:  48 bits.  The address takes the form FFXX::XXLL:PPPP:PPPP:
  //            XXXX:XXXX.  L denotes nibbles used to encode the prefix
  //            length.  P denotes nibbles used to encode the prefix itself.
  //            The prefix information is taken from the specified context.
  //         10:  reserved
  //         11:  reserved
  if(inIpv6Data[24] == 0xff)
  {
    //      1: Destination address uses multicast compression.
    if(inIpv6Data[26] == 0x00 && inIpv6Data[27] == 0x00
          && inIpv6Data[28] == 0x00 && inIpv6Data[29] == 0x00 && inIpv6Data[30] == 0x00 && inIpv6Data[31] == 0x00
          && inIpv6Data[32] == 0x00 && inIpv6Data[33] == 0x00 && inIpv6Data[34] == 0x00)
    {
      if(inIpv6Data[25] == 0x02 && inIpv6Data[35] == 0x00 && inIpv6Data[36] == 0x00 && inIpv6Data[37] == 0x00
        && inIpv6Data[38] == 0x00)
      {
        //      If M=1 and DAC=0:
        //         11:  8 bits.  The address takes the form FF02::00XX.             FF02:0000:0000:0000:0000:0000:0000:00XX
        header2 |= LOWPAN_IPHC_HEADER_2_M_BIT_MASK | ((0x03 << 0) & LOWPAN_IPHC_HEADER_2_DAM_MASK);
        payload[payloadIdx++] = inIpv6Data[39];
      }
      else if((inIpv6Data[25] & 0xf0) == 0x00 && inIpv6Data[35] == 0x00 && inIpv6Data[36] == 0x00 && inIpv6Data[37] == 0x00
        && (inIpv6Data[38] & 0xf0) == 0x00)
      {
        //      If M=1 and DAC=0:
        //         10:  16 bits.  The address takes the form FF0X::0XXX.            FF0X:0000:0000:0000:0000:0000:0000:0XXX
        header2 |= LOWPAN_IPHC_HEADER_2_M_BIT_MASK | ((0x02 << 0) & LOWPAN_IPHC_HEADER_2_DAM_MASK);
        payload[payloadIdx++] = ((inIpv6Data[25] << 4) & 0xf0) | ((inIpv6Data[38] << 0) & 0x0f);
        payload[payloadIdx++] = inIpv6Data[39];
      }
      else if(inIpv6Data[35] == 0x00 && inIpv6Data[36] == 0x00)
      {
        //      If M=1 and DAC=0:
        //         01:  32 bits.  The address takes the form FFXX::00XX:XXXX.       FFXX:0000:0000:0000:0000:0000:00XX:XXXX
        header2 |= LOWPAN_IPHC_HEADER_2_M_BIT_MASK | ((0x01 << 0) & LOWPAN_IPHC_HEADER_2_DAM_MASK);
        payload[payloadIdx++] = inIpv6Data[25];
        payload[payloadIdx++] = inIpv6Data[37];
        payload[payloadIdx++] = inIpv6Data[38];
        payload[payloadIdx++] = inIpv6Data[39];
      }
      else
      {
        //      If M=1 and DAC=0:
        //         00:  48 bits.  The address takes the form FFXX::00XX:XXXX:XXXX.  FFXX:0000:0000:0000:0000:00XX:XXXX:XXXX
        header2 |= LOWPAN_IPHC_HEADER_2_M_BIT_MASK | ((0x00 << 0) & LOWPAN_IPHC_HEADER_2_DAM_MASK);
        payload[payloadIdx++] = inIpv6Data[25];
        payload[payloadIdx++] = inIpv6Data[35];
        payload[payloadIdx++] = inIpv6Data[36];
        payload[payloadIdx++] = inIpv6Data[37];
        payload[payloadIdx++] = inIpv6Data[38];
        payload[payloadIdx++] = inIpv6Data[39];
      }
    }
    else
    {
      //      If M=0:
      //         If DAC=0:
      //            00:  128 bits.  The full address is carried in-line.
      for(i = 0; i < 16; i++)
      {
        payload[payloadIdx++] = inIpv6Data[24 + i];   //IPv6 Header Destination Address
      }
    }
  }
  //      0: Destination address does not use multicast compression.
  else if(inIpv6Data[24] == 0xfe && inIpv6Data[25] == 0x80 && inIpv6Data[26] == 0x00 && inIpv6Data[27] == 0x00
      && inIpv6Data[28] == 0x00 && inIpv6Data[29] == 0x00 && inIpv6Data[30] == 0x00 && inIpv6Data[31] == 0x00
      && inIpv6Data[32] == 0x02 && inIpv6Data[33] == 0x1e && inIpv6Data[34] == 0x32 && inIpv6Data[35] == homeIdBigEndian[0]
      && inIpv6Data[36] == homeIdBigEndian[1] && inIpv6Data[37] == homeIdBigEndian[2])
  {
    if(inIpv6Data[38] == homeIdBigEndian[3] && inIpv6Data[39] == destinationNodeId)
    {
      //      0: Destination address compression uses stateless compression.
      //            11:  0 bits.  The address is fully elided.  The first 64
      //               bits of the address are the link-local prefix padded with
      //               zeros.  The remaining 64 bits are computed from the link-
      //               layer address as 0x021E32 + HomeID + NodeID.
      header2 |= (0x03 << 0) & LOWPAN_IPHC_HEADER_2_DAM_MASK;
    }
    else
    {
      //      0: Destination address compression uses stateless compression.
      //      10:  16 bits.  The first 112 bits of the address are elided.
      //         The value of those bits is the link-local prefix padded with
      //         zeros.  The remaining 16 bits are carried inline.
      //         Z-Wave route-over: 16 bits inline
      header2 |= (0x02 << 0) & LOWPAN_IPHC_HEADER_2_DAM_MASK;
      payload[payloadIdx++] = homeIdBigEndian[3];
      payload[payloadIdx++] = sourceNodeId;
    }
  }
  else
  {
    //      If M=0:
    //         If DAC=0:
    //            00:  128 bits.  The full address is carried in-line.
    for(i = 0; i < 16; i++)
    {
      payload[payloadIdx++] = inIpv6Data[24 + i];       //IPv6 Header Destination Address
    }
  }


  // LOWPAN_NHC - IPv6 Extension Header Compression
  // compressed headers:
  prev_hext_header_idx = 0;
  next_header = inIpv6Data[6];  //IPv6 Header Next Header
  next_header_idx = 40;         //IPv6 header size
  while (next_header != 59 )    //No Next Header for IPv6
  {
    if(next_header == 0         //IPv6 Hop-by-Hop Options [RFC2460]
      || next_header == 43      //IPv6 Routing [RFC2460]
      || next_header == 60      //IPv6 Destination Options [RFC2460]
      || next_header == 135     //IPv6 Mobility Header [RFC3775]
      )
    {
      if(prev_hext_header_idx)
      {
        //NH: Next Header:
        //      0: Full 8 bits for Next Header are carried in-line.
        //      1: The Next Header field is compressed and the next header is
        //         compressed using LOWPAN_NHC, which is discussed in Section 3.
        payload[prev_hext_header_idx] |= 0x01;  //NH bit
      }
      prev_hext_header_idx = payloadIdx;

      //The Next Header Field contained in IPv6 Extension Headers is elided
      // when the NH bit is set in the LOWPAN_NHC encoding octet.  Note that
      // doing so allows LOWPAN_NHC to utilize no more overhead than the non-
      // encoded IPv6 Extension Header.
      switch (next_header)
      {
      case 0:       //IPv6 Hop-by-Hop Options [RFC2460]
        payload[payloadIdx++] = 0xe0 | ((0 << 1) & 0x0e);
        //TODO: IPv6 Hop-by-Hop and Destination Options Headers may use Pad1 and PadN... see below:
        break;
      case 43:    //IPv6 Routing [RFC2460]
        payload[payloadIdx++] = 0xe0 | ((1 << 1) & 0x0e);
        break;
      case 60:    //IPv6 Destination Options [RFC2460]
        payload[payloadIdx++] = 0xe0 | ((3 << 1) & 0x0e);
        //TODO: IPv6 Hop-by-Hop and Destination Options Headers may use Pad1 and PadN... see below:
        break;
      case 135:   //IPv6 Mobility Header [RFC3775]
        payload[payloadIdx++] = 0xe0 | ((4 << 1) & 0x0e);
        break;
      }
      //The Length Field contained in IPv6 Extension Headers indicate the
      //length of the IPv6 Extension Header in octets, not including the
      //LOWPAN_NHC byte.  Note that this changes the standard Length Field
      //definition from indicating the header size in 8-octet units, not
      //including the first 8 octets.  Changing the Length Field to be in
      //units of octets removes wasteful internal fragmentation.  However,
      //specifying units in octets also means that LOWPAN_NHC CANNOT be used
      //to encode IPv6 Extension Headers that exceed 255 octets.
      n = ((1 + inIpv6Data[next_header_idx + 1]) * 8) - 1;
      payload[payloadIdx++] = n;

      //copy header data:
      //TODO: IPv6 Hop-by-Hop and Destination Options Headers may use Pad1 and PadN
      // to pad out the header to a multiple of 8 octets in length.  When
      // using LOWPAN_NHC, those Pad1 and PadN options MAY be elided and the
      // length of the header reduced by the size of those Pad1 and PadN
      // options.  When converting from the LOWPAN_NHC encoding back to the
      // standard IPv6 encoding, Pad1 and PadN options MUST be used to pad out
      // the containing header to a multiple of 8 octets in length if
      // necessary.  Note that Pad1 and PadN options that do not appear at the
      // end of the containing header MUST NOT be elided as they are used to
      // align subsequent options.
      n--;    //skip the IPv6 Extension Header Length Field.
      for(i = 0, n--; i < n; i++)
      {
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 1 + 1 + i];  //skip the IPv6 Extension Header NextHeader Field (+1) and Length Field (+1).
      }

      //get next header parameters:
      next_header = inIpv6Data[next_header_idx];
      next_header_idx += (1 + (WORD)inIpv6Data[next_header_idx + 1]) * 8;
    }
    else if(next_header == 44)      //IPv6 Fragment [RFC2460]
    {
      if(prev_hext_header_idx)
      {
        //NH: Next Header:
        //      0: Full 8 bits for Next Header are carried in-line.
        //      1: The Next Header field is compressed and the next header is
        //         compressed using LOWPAN_NHC, which is discussed in Section 3.
        payload[prev_hext_header_idx] |= 0x01;  //NH bit
      }

      //LOWPAN_NHC byte:
      payload[payloadIdx++] = 0xe0 | ((0 << 2) & 0x0e);

      //get IPv6 Header Payload Length:
      nw = ((WORD)inIpv6Data[4] << 8) | ((WORD)inIpv6Data[5] << 0);   // IPv6 Header Payload Length

      //convert IPv6 Header Payload Length to the Fragment Header and Data Length:
      nw -= next_header_idx - 40; //40 - IPv6 header size

      //copy Fragment header and Fragment Data:
      for(iw = 0; iw < nw; iw++)
      {
        payload[payloadIdx++] = inIpv6Data[next_header_idx + iw];
      }

      //get next header parameters:
      next_header = 59;    //No Next Header for IPv6
      next_header_idx += nw;
      break;
    }
    else if(next_header == 41)       //IPv6 Header
    {
      //if(prev_hext_header_idx)
      //{
        //NH: Next Header:
        //      0: Full 8 bits for Next Header are carried in-line.
        //      1: The Next Header field is compressed and the next header is
        //         compressed using LOWPAN_NHC, which is discussed in Section 3.
      //  payload[prev_hext_header_idx] |= 0x01;  //NH bit
      //}

      //TODO: recursive IPv6 Header compression. When the identified next header is an IPv6 Header (EID=7), the NH bit
      // of the LOWPAN_NHC encoding is unused and SHOULD be set to zero.  The
      // bytes following follow the LOWPAN_IPHC encoding as defined in Section 2.

      //copy rest headers uncompressed...
      break;
    }
    else if(next_header == 17)      //UDP
    {
      if(prev_hext_header_idx)
      {
        //NH: Next Header:
        //      0: Full 8 bits for Next Header are carried in-line.
        //      1: The Next Header field is compressed and the next header is
        //         compressed using LOWPAN_NHC, which is discussed in Section 3.
        payload[prev_hext_header_idx] |= 0x01;  //NH bit
      }

      //LOWPAN_NHC byte:
      i = payloadIdx;
      payload[payloadIdx++] = 0xf0;

      //P: Ports:
      if(inIpv6Data[next_header_idx + 0] == 0xf0 && inIpv6Data[next_header_idx + 2] == 0xf0
        && (inIpv6Data[next_header_idx + 1] & 0xf0) == 0xb0 && (inIpv6Data[next_header_idx + 3] & 0xf0) == 0xb0)
      {
        //   11:  First 12 bits of both Source Port and Destination Port are
        //      0xF0B and elided.  The remaining 4 bits for each are carried
        //      in-line.
        payload[i] |= 0x03;
        payload[payloadIdx++] = ((inIpv6Data[next_header_idx + 1] << 4) & 0xf0)
                              | ((inIpv6Data[next_header_idx + 3] << 0) & 0x0f);
      }
      else if(inIpv6Data[next_header_idx + 0] == 0xf0)
      {
        //   10:  First 8 bits of Source Port are 0xF0 and elided.  The
        //      remaining 8 bits of Source Port are carried in-line.  All 16
        //      bits for Destination Port are carried in-line.
        payload[i] |= 0x02;
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 1];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 2];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 3];
      }
      else if(inIpv6Data[next_header_idx + 2] == 0xf0)
      {
        //   01:  All 16 bits for Source Port are carried in-line.  First 8
        //      bits of Destination Port is 0xF0 and elided.  The remaining 8
        //      bits of Destination Port are carried in-line.
        payload[i] |= 0x01;
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 0];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 1];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 3];
      }
      else
      {
        //   00:  All 16 bits for both Source Port and Destination Port are
        //      carried in-line.
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 0];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 1];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 2];
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 3];
      }

      //C: Checksum:
      //   0: All 16 bits of Checksum are carried in-line.
      //   1: ZW: N/A – unless 16 checksum is provided by all network nodes.
      payload[payloadIdx++] = inIpv6Data[next_header_idx + 6];
      payload[payloadIdx++] = inIpv6Data[next_header_idx + 7];

      //get UDP Payload Length:
      nw = ((WORD)inIpv6Data[4] << 8) | ((WORD)inIpv6Data[5] << 0);   // UDP Header Length field
      nw -= 8;   // UPD Header size

      //copy UDP Payload Data:
      for(iw = 0; iw < nw; iw++)
      {
        payload[payloadIdx++] = inIpv6Data[next_header_idx + 8 + iw]; // 8 - UPD Header size
      }

      //get next header parameters:
      next_header = 59;    //No Next Header for IPv6
      next_header_idx += nw;
      break;
    }
    else
    {
      break;
    }
  }

  // store uncompressed headers:
  if (next_header != 59 ) //No Next Header for IPv6
  {
    //get IPv6 Header Payload Length:
    nw = ((WORD)inIpv6Data[4] << 8) | ((WORD)inIpv6Data[5] << 0);   // IPv6 Header Payload Length

    //convert IPv6 Header Payload Length to the uncompressed headers length:
    nw -= next_header_idx - 40; //40 - IPv6 header size

    //copy uncompressed headers to the output buffer:
    for(iw = 0; iw < nw; iw++)
    {
      payload[payloadIdx++] = inIpv6Data[next_header_idx + iw];
    }
  }

  //Update Return variables:
  if(payloadIdx > payloadLenMax)
  {
    //TODO: before each increment of payloadIdx we should check, if it not greater than payloadLenMax!
  }
  *outLowPanDataSize = payloadIdx;
}

// IPv6 <-> 6lowPAN IPHC converters: end code

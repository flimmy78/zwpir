/**********************  ZW_classcmd_ex.h  *******************************
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
 * Description:   Additional defines of Z-Wave Library constants,
 *                 which are not exist in ZW_classcmd.h
 *
 * Author:      Valeriy Vyshnyak
 *
 * Last Changed By:  $Author: vvi $
 * Revision:         $Revision: 0000 $
 * Last Changed:     $Date: Sep 11, 2009 12:27:48 PM $
 *
 ****************************************************************************/


#ifndef ZW_CLASSCMD_EX_H_
#define ZW_CLASSCMD_EX_H_


#define ZW_COMMON_FRAME_SIZE                        2
#define ZW_COMMAND_ZIP_GATEWAY_SET_FRAME_SIZE       3
#define ZW_COMMAND_ZIP_SUBNET_SET_FRAME_SIZE        19
#define ZW_SWITCH_MULTILEVEL_SET_FRAME_SIZE         3
#define ZW_SWITCH_MULTILEVEL_GET_FRAME_SIZE         2

#define COMMAND_ZIP_PACKET_HEADER_SIZE              5 //cmdClass, cmd, properties1, 2, seqNo
#define COMMAND_ZIP_PACKET_HOME_ID_SIZE             4
#define COMMAND_ZIP_PACKET_NODE_IDS_SIZE            2
#define COMMAND_ZIP_PACKET_GATEWAY_MAC_SIZE         6
#define COMMAND_ZIP_PACKET_CUSTOMER_ID_SIZE         8
#define COMMAND_ZIP_PACKET_WEB_KEEPALIVE_DELAY_SIZE 1

#define COMMAND_ZIP_SUBNET_SET                      0x01
/************************************************************/
/* Z/IP Subnet Set command  */
/************************************************************/
typedef struct _ZW_COMMAND_ZIP_SUBNET_SET_FRAME_
{
    BYTE      cmdClass;                     /* The command class */
    BYTE      cmd;                          /* The command */
    BYTE      properties1;
    BYTE      ipAddressByte0;
    BYTE      ipAddressByte1;
    BYTE      ipAddressByte2;
    BYTE      ipAddressByte3;
    BYTE      ipAddressByte4;
    BYTE      ipAddressByte5;
    BYTE      ipAddressByte6;
    BYTE      ipAddressByte7;
    BYTE      ipAddressByte8;
    BYTE      ipAddressByte9;
    BYTE      ipAddressByte10;
    BYTE      ipAddressByte11;
    BYTE      ipAddressByte12;
    BYTE      ipAddressByte13;
    BYTE      ipAddressByte14;
    BYTE      ipAddressByte15;
} ZW_COMMAND_ZIP_SUBNET_SET_FRAME;

#define COMMAND_ZIP_SUBNET_SET_PROPERTIES1_IPV6_MASK  0x01
#define COMMAND_ZIP_SUBNET_SET_HEADER_SIZE            3
#define COMMAND_ZIP_SUBNET_SET_IPV4_ADDRESS_SIZE      4
#define COMMAND_ZIP_SUBNET_SET_IPV6_ADDRESS_SIZE      16



#define LOWPAN_FIRST_FRAGMENT_HEADER_SIZE             4   //cmdClass, cmd_datagramSize1, datagramSize2, datagramTag

#define LOWPAN_SUBSEQUENT_FRAGMENT_HEADER_SIZE        5   //cmdClass, cmd_datagramSize1, datagramSize2, datagramTag, datagramOffset
#define LOWPAN_SUBSEQUENT_FRAGMENT_DATAGRAM_OFFSET_UNIT_SIZE    8

#define IPV6_HEADER_SIZE                                  2   //cmdClass, cmd
#define IPV6_6LOWPAN_HEADER_OFFSET                        1   //cmdClass byte should be ignored
#define IPV6_6LOWPAN_HEADER_SIZE                          1   //cmd

#define LOWPAN_IPHC_HEADER_SIZE_MIN                       3   //cmdClass, cmd_header1, header2
#define LOWPAN_IPHC_HEADER_SIZE_MAX                       4   //cmdClass, cmd_header1, header2, header3
#define LOWPAN_IPHC_6LOWPAN_HEADER_OFFSET                 1   //cmdClass byte should be ignored
#define LOWPAN_IPHC_6LOWPAN_HEADER_SIZE_MIN               2   //cmd_header1, header2
#define LOWPAN_IPHC_6LOWPAN_HEADER_SIZE_MAX               3   //cmd_header1, header2, header3

#endif /* ZW_CLASSCMD_EX_H_ */

/*
 * Driver interaction with extended Linux Wireless Extensions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */
#ifndef DRIVER_CMD_WEXT_H
#define DRIVER_CMD_WEXT_H

#define WEXT_NUMBER_SCAN_CHANNELS_FCC	11
#define WEXT_NUMBER_SCAN_CHANNELS_ETSI	13
#define WEXT_NUMBER_SCAN_CHANNELS_MKK1	14

#define RSSI_CMD			"RSSI"
#define LINKSPEED_CMD			"LINKSPEED"

#define WPA_DRIVER_WEXT_WAIT_US		400000
#define MAX_DRV_CMD_SIZE		248
#define WEXT_NUMBER_SEQUENTIAL_ERRORS	4
#define WEXT_CSCAN_AMOUNT		9
#define WEXT_CSCAN_BUF_LEN		360
#define WEXT_CSCAN_HEADER		"CSCAN S\x01\x00\x00S\x00"
#define WEXT_CSCAN_HEADER_SIZE		12
#define WEXT_CSCAN_SSID_SECTION		'S'
#define WEXT_CSCAN_CHANNEL_SECTION	'C'
#define WEXT_CSCAN_NPROBE_SECTION	'N'
#define WEXT_CSCAN_ACTV_DWELL_SECTION	'A'
#define WEXT_CSCAN_PASV_DWELL_SECTION	'P'
#define WEXT_CSCAN_HOME_DWELL_SECTION	'H'
#define WEXT_CSCAN_TYPE_SECTION		'T'
#define WEXT_CSCAN_TYPE_DEFAULT		0
#define WEXT_CSCAN_TYPE_PASSIVE		1
#define WEXT_CSCAN_PASV_DWELL_TIME	130
#define WEXT_CSCAN_PASV_DWELL_TIME_DEF	250
#define WEXT_CSCAN_PASV_DWELL_TIME_MAX	3000
#define WEXT_CSCAN_HOME_DWELL_TIME	130

#define WEXT_PNOSETUP_HEADER            "PNOSETUP "
#define WEXT_PNOSETUP_HEADER_SIZE       9
#define WEXT_PNO_TLV_PREFIX             'S'
#define WEXT_PNO_TLV_VERSION            '1'
#define WEXT_PNO_TLV_SUBVERSION         '2'
#define WEXT_PNO_TLV_RESERVED           '0'
#define WEXT_PNO_VERSION_SIZE           4
#define WEXT_PNO_AMOUNT                 16
#define WEXT_PNO_SSID_SECTION           'S'
/* SSID header size is SSID section type above + SSID length */
#define WEXT_PNO_SSID_HEADER_SIZE       2
#define WEXT_PNO_SCAN_INTERVAL_SECTION  'T'
#define WEXT_PNO_SCAN_INTERVAL_LENGTH   2
#define WEXT_PNO_SCAN_INTERVAL          30
/* Scan interval size is scan interval section type + scan interval length above*/
#define WEXT_PNO_SCAN_INTERVAL_SIZE     (1 + WEXT_PNO_SCAN_INTERVAL_LENGTH)
#define WEXT_PNO_REPEAT_SECTION         'R'
#define WEXT_PNO_REPEAT_LENGTH          1
#define WEXT_PNO_REPEAT                 4
/* Repeat section size is Repeat section type + Repeat value length above*/
#define WEXT_PNO_REPEAT_SIZE            (1 + WEXT_PNO_REPEAT_LENGTH)
#define WEXT_PNO_MAX_REPEAT_SECTION     'M'
#define WEXT_PNO_MAX_REPEAT_LENGTH      1
#define WEXT_PNO_MAX_REPEAT             3
/* Max Repeat section size is Max Repeat section type + Max Repeat value length above*/
#define WEXT_PNO_MAX_REPEAT_SIZE        (1 + WEXT_PNO_MAX_REPEAT_LENGTH)
/* This corresponds to the size of all sections expect SSIDs */
#define WEXT_PNO_NONSSID_SECTIONS_SIZE  (WEXT_PNO_SCAN_INTERVAL_SIZE + WEXT_PNO_REPEAT_SIZE + WEXT_PNO_MAX_REPEAT_SIZE)
/* PNO Max command size is total of header, version, ssid and other sections + Null termination */
#define WEXT_PNO_MAX_COMMAND_SIZE       (WEXT_PNOSETUP_HEADER_SIZE + WEXT_PNO_VERSION_SIZE \
					+ WEXT_PNO_AMOUNT * (WEXT_PNO_SSID_HEADER_SIZE + IW_ESSID_MAX_SIZE) \
					+ WEXT_PNO_NONSSID_SECTIONS_SIZE + 1)

#endif /* DRIVER_CMD_WEXT_H */

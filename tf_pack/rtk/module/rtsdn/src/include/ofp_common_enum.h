/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Estinet Technologies Inc. (C) 2014
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("ESTINET SOFTWARE")
 *  RECEIVED FROM ESTINET AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. ESTINET EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES ESTINET PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE ESTINET SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. ESTINET SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY ESTINET SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND ESTINET'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE ESTINET SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT ESTINET'S OPTION, TO REVISE OR REPLACE THE ESTINET SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  ESTINET FOR SUCH ESTINET SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
 *  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
 *
 *******************************************************************************/

#ifndef __OFP_COMMON_ENUM_H__
#define __OFP_COMMON_ENUM_H__

#include <unistd.h>

/* Enumerations in the file are prefixed with "EN_"
 * to avoid duplicate definitions with OpenFlow
 * agent software (ex, Open vSwitch)
 */
#ifndef ETH_ADDR_LEN
#define ETH_ADDR_LEN 6
#endif

#define OFP_ETH_ALEN		6
#define OFP_MAX_PORT_NAME_LEN	16
#define OFP_MAX_TABLE_NAME_LEN	32

#define DESC_STR_LEN		256
#define SERIAL_NUM_LEN		32

#define OFP_NO_BUFFER		0xffffffff

#define OFPQ_ALL		0xffffffff
#define OFPQ_MAX_RATE_UNCFG	0xffff
#define OFPQ_MIN_RATE_UNCFG	0xffff

typedef enum en_vm_tag_type
{
	EN_VM_TAG_NONE			= 0x0B00,
	EN_VM_S_TAG,
	EN_VM_E_TAG,
	EN_VM_VN_TAG,
}EN_VM_TAG_TYPE_T;

typedef enum en_ofp_counter_type
{
	/* Per Table */
	EN_OFPC_TABLE_REFERENCE		= 0x00,
	EN_OFPC_TABLE_MATCHES,
	EN_OFPC_TABLE_LOOKUPS,

	/* Per Flow Entry */
	EN_OFPC_FLOW_PACKETS_RECEIVED	= 0x10,
	EN_OFPC_FLOW_BYTES_RECEIVED,
	EN_OFPC_FLOW_DURATION_SECS,
	EN_OFPC_FLOW_DURATION_NSECS,

	/* Per Port */
	EN_OFPC_PORT_PACKETS_RECEIVED	= 0x20,
	EN_OFPC_PORT_PACKETS_TRANSMITTED,
	EN_OFPC_PORT_BYTES_RECEIVED,
	EN_OFPC_PORT_BYTES_TRANSMITTED,
	EN_OFPC_PORT_DROPS_RECEIVED,
	EN_OFPC_PORT_DROPS_TRANSMITTED,
	EN_OFPC_PORT_ERRORS_RECEIVED,
	EN_OFPC_PORT_ERRORS_TRANSMITTED,
	EN_OFPC_PORT_FA_ERRORS_RECEIVED,
	EN_OFPC_PORT_OR_ERRORS_RECEIVED,
	EN_OFPC_PORT_CRC_ERRORS_RECEIVED,
	EN_OFPC_PORT_COLLISIONS,
	EN_OFPC_PORT_DURATION_SECS,
	EN_OFPC_PORT_DURATION_NSECS,

	/* Per Queue */
	EN_OFPC_QUEUE_PACKETS_TRANSMITTED= 0x30,
	EN_OFPC_QUEUE_BYTES_TRANSMITTED,
	EN_OFPC_QUEUE_OR_TRANSMITTER,
	EN_OFPC_QUEUE_DURATION_SECS,
	EN_OFPC_QUEUE_DURATION_NSECS,

	/* Per Group */
	EN_OFPC_GROUP_REFERENCE		= 0x40,
	EN_OFPC_GROUP_PACKETS_COUNT,
	EN_OFPC_GROUP_BYTES_COUNT,
	EN_OFPC_GROUP_DURATION_SECS,
	EN_OFPC_GROUP_DURATION_NSECS,

	/* Per Group Bucket */
	EN_OFPC_BUCKET_PACKETS_COUNT	= 0x50,
	EN_OFPC_BUCKET_BYTES_COUNT,

	/* Per Meter */
	EN_OFPC_METER_ENTRY_FLOW_COUNT	= 0x60,
	EN_OFPC_METER_ENTRY_INPUT_PACKET,
	EN_OFPC_METER_ENTRY_INPUT_BYTE,
	EN_OFPC_METER_ENTRY_DURATION_SECS,
	EN_OFPC_METER_ENTRY_DURATION_NSECS,

	/* Per Meter Band */
	EN_OFPC_METER_BAND_PACKET_COUNT	= 0x70,
	EN_OFPC_METER_BAND_BYTE_COUNT,
}EN_OFP_COUNTER_TYPE_T;

/*
 * Handling of IP fragments.
 *
 * @OFPC_FRAG_NORMAL        No special handling for fragments.
 * @OFPC_FRAG_DROP          Drop fragments.
 * @OFPC_FRAG_REASM         Reassemble (only if OFPC_IP_REASM set).
 * @OFPC_FRAG_MASK
 */
typedef enum en_ofp_config_flags
{
	EN_OFPC_FRAG_NORMAL		= 0,
	EN_OFPC_FRAG_DROP		= 1 << 0,
	EN_OFPC_FRAG_REASM		= 1 << 1,
	EN_OFPC_FRAG_MASK		= 3,
}EN_OFP_CONFIG_FLAGS_T;

/*
 *
 * @OFPC_FLOW_STATS         Flow statistics.
 * @OFPC_TABLE_STATS        Table statistics.
 * @OFPC_PORT_STATS         Port statistics.
 * @OFPC_GROUP_STATS        Group statistics.
 * @OFPC_IP_REASM           Can reassemble IP fragments.
 * @OFPC_QUEUE_STATS        Queue statistics.
 * @OFPC_PORT_BLOCKED       Switch will block looping ports.
 */
typedef enum en_ofp_capabilities
{
	EN_OFPC_FLOW_STATS		= 1 << 0,
	EN_OFPC_TABLE_STATS		= 1 << 1,
	EN_OFPC_PORT_STATS		= 1 << 2,
	EN_OFPC_GROUP_STATS		= 1 << 3,
	EN_OFPC_IP_REASM		= 1 << 5,
	EN_OFPC_QUEUE_STATS		= 1 << 6,
	EN_OFPC_PORT_BLOCKED		= 1 << 8,
}EN_OFP_CAPABILITIES_T;

/*
 * @OFPP_MAX                Maximum number of physical and logical switch ports.

 * Reserved OpenFlow Port (fake output "ports").
 * @OFPP_IN_PORT            Send the packet out the input port. This reserved
                            port must be explicitly used in order to send back
                            out of the input port.
 * @OFPP_TABLE              Submit the packet to the first flow table
                            NB: This destination port can only be used in
                            packet-out messages.
 * @OFPP_NORMAL             Process with normal L2/L3 switching.
 * @OFPP_FLOOD              All physical ports in VLAN, except input port and
                            those blocked or link down.
 * @OFPP_ALL                All physical ports except input port.
 * @OFPP_CONTROLLER         Send to controller.
 * @OFPP_LOCAL              Local openflow "port".
 * @OFPP_ANY                Wildcard port used only for flow mod (delete) and
                            flow stats requests. Selects all flows regardless
                            of output port (including flows with no output port)
*/
typedef enum en_ofp_port_no
{
	EN_OFPP_MAX			= 0xFFFFFF00,
	EN_OFPP_IN_PORT			= 0xFFFFFFF8,
	EN_OFPP_TABLE			= 0xFFFFFFF9,
	EN_OFPP_NORMAL			= 0xFFFFFFFA,
	EN_OFPP_FLOOD			= 0xFFFFFFFB,
	EN_OFPP_ALL			= 0xFFFFFFFC,
	EN_OFPP_CONTROLLER		= 0xFFFFFFFD,
	EN_OFPP_LOCAL			= 0xFFFFFFFE,
	EN_OFPP_ANY			= 0xFFFFFFFF,
}EN_OFP_PORT_NO_T;

/*
 * @OFPPS_LINK_DOWN     No physical link present.
 * @OFPPS_BLOCKED       Port is blocked
 * @OFPPS_LIVE          Live for Fast Failover Group.
 */
typedef enum en_ofp_port_state
{
	EN_OFPPS_LINK_DOWN		= 1 << 0,
	EN_OFPPS_BLOCKED		= 1 << 1,
	EN_OFPPS_LIVE			= 1 << 2,
}EN_OFP_PORT_STATE_T;

/*
 * @OFPPF_10MB_HD       10 Mb half-duplex rate support.
 * @OFPPF_10MB_FD       10 Mb full-duplex rate support.
 * @OFPPF_100MB_HD      100 Mb half-duplex rate support.
 * @OFPPF_100MB_FD      100 Mb full-duplex rate support.
 * @OFPPF_1GB_HD        1 Gb half-duplex rate support.
 * @OFPPF_1GB_FD        1 Gb full-duplex rate support.
 * @OFPPF_10GB_FD       10 Gb full-duplex rate support.
 * @OFPPF_40GB_FD       40 Gb full-duplex rate support.
 * @OFPPF_100GB_FD      100 Gb full-duplex rate support.
 * @OFPPF_1TB_FD        1 Tb full-duplex rate support.
 * @OFPPF_OTHER         Other rate, not in the list.
 * @OFPPF_COPPER        Copper medium.
 * @OFPPF_FIBER         iber medium.
 * @OFPPF_AUTONEG       Auto-negotiation.
 * @OFPPF_PAUSE         Pause.
 * @OFPPF_PAUSE_ASYM    Asymmetric pause.
 */
#if 0
//already defined in sdn_hal.h
typedef enum en_ofp_port_features
{
	EN_OFPPF_10MB_HD		= 1 << 0,
	EN_OFPPF_10MB_FD		= 1 << 1,
	EN_OFPPF_100MB_HD		= 1 << 2,
	EN_OFPPF_100MB_FD		= 1 << 3,
	EN_OFPPF_1GB_HD			= 1 << 4,
	EN_OFPPF_1GB_FD			= 1 << 5,
	EN_OFPPF_10GB_FD		= 1 << 6,
	EN_OFPPF_40GB_FD		= 1 << 7,
	EN_OFPPF_100GB_FD		= 1 << 8,
	EN_OFPPF_1TB_FD			= 1 << 9,
	EN_OFPPF_OTHER			= 1 << 10,
	EN_OFPPF_COPPER			= 1 << 11,
	EN_OFPPF_FIBER			= 1 << 12,
	EN_OFPPF_AUTONEG		= 1 << 13,
	EN_OFPPF_PAUSE			= 1 << 14,
	EN_OFPPF_PAUSE_ASYM		= 1 << 15,
}EN_OFP_PORT_FEATURES_T;
#endif
/*
 * @OFPPC_PORT_DOWN         Port is administratively down.
 * @OFPPC_NO_RECV           Drop all packets received by port.
 * @OFPPC_NO_FWD            Drop packets forwarded to port.
 * @OFPPC_NO_PACKET_IN      Do not send packet-in msgs for port.
 *
 * EstiNet definitions
 * @OFPPC_PORT_BIDIR        Port is bidirection
 * @OFPPC_PORT_INGRESS      Port is ingress port
 * @OFPPC_PORT_EGRESS       Port is egress port
*/

#if 0
//already defined in sdn_hal.h
typedef enum en_ofp_port_config
{
	EN_OFPPC_PORT_DOWN		= 1 << 0,
	EN_OFPPC_NO_STP			= 1 << 1,
	EN_OFPPC_NO_RECV		= 1 << 2,
	EN_OFPPC_NO_RECV_STP		= 1 << 3,
	EN_OFPPC_NO_FLOOD		= 1 << 4,
	EN_OFPPC_NO_FWD			= 1 << 5,
	EN_OFPPC_NO_PACKET_IN		= 1 << 6,

	/*
	 * EstiNet defined
	 */
	EN_OFPPC_PORT_BIDIR		= 1 << 9,
	EN_OFPPC_PORT_INGRESS		= 1 << 10,
	EN_OFPPC_PORT_EGRESS		= 1 << 11,
}EN_OFP_PORT_CONFIG_T;
#endif

/* What changed about the physical port
 * @OFPPR_ADD           The port was added.
 * @OFPPR_DELETE        The port was removed.
 * @OFPPR_MODIFY        Some attribute of the port has changed.
 */
typedef enum en_ofp_port_reason
{
    EN_OFPPR_ADD        = 0,
    EN_OFPPR_DELETE     = 1,
    EN_OFPPR_MODIFY     = 2,
}EN_OFP_PORT_REASON_T;

/*
 * @OFPQT_MIN_RATE          Minimum datarate guaranteed.
 * @OFPQT_MAX_RATE          Maximum datarate.
 * @OFPQT_EXPERIMENTER      Experimenter defined property.
 */
typedef enum en_ofp_queue_properties
{
    EN_OFPQT_MIN_RATE       = 1,
    EN_OFPQT_MAX_RATE       = 2,
    EN_OFPQT_EXPERIMENTER   = 0xFFFF,
}EN_OFP_QUEUE_PROPERTIES_T;

/*
 * @OFPAT_OUTPUT            Output to switch port.
 * @OFPAT_COPY_TTL_OUT      Copy TTL "outwards" -- from next-to-outermost to outermost
 * @OFPAT_COPY_TTL_IN       Copy TTL "inwards" -- from outermost to next-to-outermost
 * @OFPAT_SET_MPLS_TTL      MPLS TTL
 * @OFPAT_DEC_MPLS_TTL      Decrement MPLS TTL
 * @OFPAT_PUSH_VLAN         Push a new VLAN tag
 * @OFPAT_POP_VLAN          Pop the outer VLAN tag
 * @OFPAT_PUSH_MPLS         Push a new MPLS tag
 * @OFPAT_POP_MPLS          Pop the outer MPLS tag
 * @OFPAT_SET_QUEUE         Set queue id when outputting to a port
 * @OFPAT_GROUP             Apply group.
 * @OFPAT_SET_NW_TTL        IP TTL.
 * @OFPAT_DEC_NW_TTL        Decrement IP TTL.
 * @OFPAT_SET_FIELD         Set a header field using OXM TLV format.
 * @OFPAT_PUSH_PBB          Push a new PBB service tag (I-TAG)
 * @OFPAT_POP_PBB           Pop the outer PBB service tag (I-TAG)
 * @OFPAT_EXPERIMENTER
 * @OFPAT_COPY_FIELD      Copy value between header and register
 * @OFPAT_METER              Apply meter (rate limiter)
 */
typedef enum en_ofp_action_type
{
    EN_OFPAT_OUTPUT         = 0,
    EN_OFPAT_COPY_TTL_OUT   = 11,
    EN_OFPAT_COPY_TTL_IN    = 12,
    EN_OFPAT_SET_MPLS_TTL   = 15,
    EN_OFPAT_DEC_MPLS_TTL   = 16,
    EN_OFPAT_PUSH_VLAN      = 17,
    EN_OFPAT_POP_VLAN       = 18,
    EN_OFPAT_PUSH_MPLS      = 19,
    EN_OFPAT_POP_MPLS       = 20,
    EN_OFPAT_SET_QUEUE      = 21,
    EN_OFPAT_GROUP          = 22,
    EN_OFPAT_SET_NW_TTL     = 23,
    EN_OFPAT_DEC_NW_TTL     = 24,
    EN_OFPAT_SET_FIELD      = 25,
    EN_OFPAT_PUSH_PBB       = 26,
    EN_OFPAT_POP_PBB        = 27,
    EN_OFPAT_COPY_FIELD     = 28,
    EN_OFPAT_METER          = 29,

    /*
     * EstiNet defined
     */
    EN_OFPAT_STD_MAX,
    EN_OFPAT_EXP_PUSH_8021BR    = EN_OFPAT_STD_MAX,
    EN_OFPAT_EXP_POP_8021BR,
    EN_OFPAT_EXP_PUSH_NIV,
    EN_OFPAT_EXP_POP_NIV,
    EN_OFPAT_EXP_PUSH_8021QBG,
    EN_OFPAT_EXP_POP_8021QBG,

    EN_OFPAT_EXPERIMENTER       = 0xFFFF,
}EN_OFP_ACTION_TYPE_T;

/*
 * @OFPMT_STANDARD      Deprecated.
 * @OFPMT_OXM           OpenFlow Extensible Match
 */
typedef enum en_ofp_match_type
{
    EN_OFPMT_STANDARD		= 0,
    EN_OFPMT_OXM			= 1,
}EN_OFP_MATCH_TYPE_T;

/* OXM Class IDs.
 * The high order bit differentiate reserved classes from member classes.
 * Classes 0x0000 to 0x7FFF are member classes, allocated by ONF.
 * Classes 0x8000 to 0xFFFE are reserved classes, reserved for standardisation.
 *
 * @OFPXMC_NXM_0                Backward compatibility with NXM
 * @OFPXMC_NXM_1                Backward compatibility with NXM
 * @OFPXMC_OPENFLOW_BASIC       Basic class for OpenFlow
 * @OFPXMC_EXPERIMENTER         Experimenter class
 */
typedef enum en_ofp_oxm_class
{
    EN_OFPXMC_NXM_0             = 0x0000,
    EN_OFPXMC_NXM_1             = 0x0001,
    EN_OFPXMC_OPENFLOW_BASIC    = 0x8000,
    EN_OFPXMC_EXPERIMENTER      = 0xFFFF,
}EN_OFP_OXM_CLASS_T;

/* OXM Flow match field types for OpenFlow basic class.
 * @OFPXMT_OFB_IN_PORT              Switch input port.
 * @OFPXMT_OFB_IN_PHY_PORT          Switch physical input port.
 * @OFPXMT_OFB_METADATA             Metadata passed between tables.
 * @OFPXMT_OFB_ETH_DST              Ethernet destination address.
 * @OFPXMT_OFB_ETH_SRC              Ethernet source address.
 * @OFPXMT_OFB_ETH_TYPE             Ethernet frame type.
 * @OFPXMT_OFB_VLAN_VID             VLAN id.
 * @OFPXMT_OFB_VLAN_PCP             VLAN priority.
 * @OFPXMT_OFB_IP_DSCP              IP DSCP (6 bits in ToS field).
 * @OFPXMT_OFB_IP_ECN               IP ECN (2 bits in ToS field).
 * @OFPXMT_OFB_IP_PROTO             IP protocol.
 * @OFPXMT_OFB_IPV4_SRC             IPv4 source address.
 * @OFPXMT_OFB_IPV4_DST             IPv4 destination address.
 * @OFPXMT_OFB_TCP_SRC              TCP source port.
 * @OFPXMT_OFB_TCP_DST              TCP destination port.
 * @OFPXMT_OFB_UDP_SRC              UDP source port.
 * @OFPXMT_OFB_UDP_DST              UDP destination port.
 * @OFPXMT_OFB_SCTP_SRC             SCTP source port.
 * @OFPXMT_OFB_SCTP_DST             SCTP destination port.
 * @OFPXMT_OFB_ICMPV4_TYPE          ICMP type.
 * @OFPXMT_OFB_ICMPV4_CODE          ICMP code.
 * @OFPXMT_OFB_ARP_OP               ARP opcode.
 * @OFPXMT_OFB_ARP_SPA              ARP source IPv4 address.
 * @OFPXMT_OFB_ARP_TPA              ARP target IPv4 address.
 * @OFPXMT_OFB_ARP_SHA              ARP source hardware address.
 * @OFPXMT_OFB_ARP_THA              ARP target hardware address.
 * @OFPXMT_OFB_IPV6_SRC             IPv6 source address.
 * @OFPXMT_OFB_IPV6_DST             IPv6 destination address.
 * @OFPXMT_OFB_IPV6_FLABEL          IPv6 Flow Label
 * @OFPXMT_OFB_ICMPV6_TYPE          ICMPv6 type.
 * @OFPXMT_OFB_ICMPV6_CODE          ICMPv6 code.
 * @OFPXMT_OFB_IPV6_ND_TARGET       Target address for ND.
 * @OFPXMT_OFB_IPV6_ND_SLL          Source link-layer for ND.
 * @OFPXMT_OFB_IPV6_ND_TLL          Target link-layer for ND.
 * @OFPXMT_OFB_MPLS_LABEL           MPLS label.
 * @OFPXMT_OFB_MPLS_TC              MPLS TC.
 * @OFPXMT_OFP_MPLS_BOS             MPLS BoS bit.
 * @OFPXMT_OFB_PBB_ISID             PBB I-SID.
 * @OFPXMT_OFB_TUNNEL_ID            Logical Port Metadata.
 * @OFPXMT_OFB_IPV6_EXTHDR          IPv6 Extension Header pseudo-field
 */
typedef enum en_oxm_ofb_match_fields
{
    EN_OFPXMT_OFB_IN_PORT		= 0,
    EN_OFPXMT_OFB_IN_PHY_PORT	= 1,
    EN_OFPXMT_OFB_METADATA		= 2,
    EN_OFPXMT_OFB_ETH_DST		= 3,
    EN_OFPXMT_OFB_ETH_SRC		= 4,
    EN_OFPXMT_OFB_ETH_TYPE		= 5,
    EN_OFPXMT_OFB_VLAN_VID		= 6,
    EN_OFPXMT_OFB_VLAN_PCP		= 7,
    EN_OFPXMT_OFB_IP_DSCP		= 8,
    EN_OFPXMT_OFB_IP_ECN		= 9,
    EN_OFPXMT_OFB_IP_PROTO		= 10,
    EN_OFPXMT_OFB_IPV4_SRC		= 11,
    EN_OFPXMT_OFB_IPV4_DST		= 12,
    EN_OFPXMT_OFB_TCP_SRC		= 13,
    EN_OFPXMT_OFB_TCP_DST		= 14,
    EN_OFPXMT_OFB_UDP_SRC		= 15,
    EN_OFPXMT_OFB_UDP_DST		= 16,
    EN_OFPXMT_OFB_SCTP_SRC		= 17,
    EN_OFPXMT_OFB_SCTP_DST		= 18,
    EN_OFPXMT_OFB_ICMPV4_TYPE	= 19,
    EN_OFPXMT_OFB_ICMPV4_CODE	= 20,
    EN_OFPXMT_OFB_ARP_OP		= 21,
    EN_OFPXMT_OFB_ARP_SPA		= 22,
    EN_OFPXMT_OFB_ARP_TPA		= 23,
    EN_OFPXMT_OFB_ARP_SHA		= 24,
    EN_OFPXMT_OFB_ARP_THA		= 25,
    EN_OFPXMT_OFB_IPV6_SRC		= 26,
    EN_OFPXMT_OFB_IPV6_DST		= 27,
    EN_OFPXMT_OFB_IPV6_FLABEL	= 28,
    EN_OFPXMT_OFB_ICMPV6_TYPE	= 29,
    EN_OFPXMT_OFB_ICMPV6_CODE	= 30,
    EN_OFPXMT_OFB_IPV6_ND_TARGET= 31,
    EN_OFPXMT_OFB_IPV6_ND_SLL	= 32,
    EN_OFPXMT_OFB_IPV6_ND_TLL	= 33,
    EN_OFPXMT_OFB_MPLS_LABEL	= 34,
    EN_OFPXMT_OFB_MPLS_TC		= 35,
    EN_OFPXMT_OFB_MPLS_BOS		= 36,
    EN_OFPXMT_OFB_PBB_ISID		= 37,
    EN_OFPXMT_OFB_TUNNEL_ID		= 38,
    EN_OFPXMT_OFB_IPV6_EXTHDR	= 39,

    /*
     * Realtek defined
     */
    EN_OFPXMT_OFB_MAX,
    EN_OFPXMT_RTK_EXP_GTP_TEID	= EN_OFPXMT_OFB_MAX,
    EN_OFPXMT_RTK_OUTER_IP_VER,
    EN_OFPXMT_RTK_OUTER_TCP_FLG,
    EN_OFPXMT_RTK_EXP_GTP_MT,
    EN_OFPXMT_RTK_OUTER_IP_FLG,
    EN_OFPXMT_RTK_INNER_IP_VER,
    EN_OFPXMT_RTK_INNER_L4DP,
    EN_OFPXMT_RTK_INNER_SIP,
    EN_OFPXMT_RTK_INNER_DIP,
    EN_OFPXMT_RTK_OUTER_FRAG_PKT,
    EN_OFPXMT_RTK_INNER_IP_PROTO,
    EN_OFPXMT_RTK_INNER_TCP_FLAG,
    EN_OFPXMT_RTK_OVER_PKT_SIZE,
    EN_OFPXMT_RTK_L2_INNER_IP_VER,
    EN_OFPXMT_RTK_EXP_MAX,

    /*
     * Estinet defined
     */
    EN_OFPXMT_EXP_VM_TAG		= EN_OFPXMT_RTK_EXP_MAX,
//    EN_OFPXMT_EXP_VM_TAG        = 50,
    EN_OFPXMT_EXP_VM_ID         = 51,
    EN_OFPXMT_EXP_VM_SEGMENT_ID = 52,
    EN_OFPXMT_MAX,
}EN_OFP_OXM_OFB_MATCH_FIELDS_T;

/* The VLAN id is 12-bits, so we can use the entire 16 bits to indicate
 * special conditions.
 *
 * @OFPVID_PRESENT      Bit that indicate that a VLAN id is set
 * @OFPVID_NONE         No VLAN id was set.
 */
typedef enum en_ofp_vlan_id
{
    EN_OFPVID_PRESENT   = 0x1000,
    EN_OFPVID_NONE      = 0x0000,
}EN_OFP_VLAN_ID_T;

/* Bit definitions for IPv6 Extension Header pseudo-field.
 *
 * @OFPIEH_NONEXT           "No next header" encountered.
 * @OFPIEH_ESP              Encrypted Sec Payload header present.
 * @OFPIEH_AUTH             Authentication header present.
 * @OFPIEH_DEST             1 or 2 dest headers present.
 * @OFPIEH_FRAG             Fragment header present.
 * @OFPIEH_ROUTER           Router header present.
 * @OFPIEH_HOP              Hop-by-hop header present.
 * @OFPIEH_UNREP            Unexpected repeats encountered.
 * @OFPIEH_UNSEQ            Unexpected sequencing encountered.
 */
typedef enum en_ofp_ipv6exthdr_flags
{
    EN_OFPIEH_NONEXT    = 1 << 0,
    EN_OFPIEH_ESP       = 1 << 1,
    EN_OFPIEH_AUTH      = 1 << 2,
    EN_OFPIEH_DEST      = 1 << 3,
    EN_OFPIEH_FRAG      = 1 << 4,
    EN_OFPIEH_ROUTER    = 1 << 5,
    EN_OFPIEH_HOP       = 1 << 6,
    EN_OFPIEH_UNREP     = 1 << 7,
    EN_OFPIEH_UNSEQ     = 1 << 8,
}EN_OFP_IPV6EXTHDR_FLAGS_T;

/*
 * @OFPIT_GOTO_TABLE        Setup the next table in the lookup pipeline
 * @OFPIT_WRITE_METADATA    Setup the metadata field for use later in pipeline
 * @OFPIT_WRITE_ACTIONS     Write the action(s) onto the datapath action set
 * @OFPIT_APPLY_ACTIONS     Applies the action(s) immediately
 * @OFPIT_CLEAR_ACTIONS     Clears all actions from the datapath action set
 * @OFPIT_METER             Apply meter (rate limiter)
 * @OFPIT_EXPERIMENTER      Experimenter instruction
 */
typedef enum en_ofp_instruction_type
{
    EN_OFPIT_GOTO_TABLE     = 1,
    EN_OFPIT_WRITE_METADATA = 2,
    EN_OFPIT_WRITE_ACTIONS  = 3,
    EN_OFPIT_APPLY_ACTIONS  = 4,
    EN_OFPIT_CLEAR_ACTIONS  = 5,
    EN_OFPIT_METER          = 6,
    EN_OFPIT_STAT_TRIGGER   = 7,      /* Statistics triggers */
    EN_OFPIT_MAX            = 8,      /* Estinet defined */
    EN_OFPIT_EXPERIMENTER   = 0xFFFF,
}EN_OFP_INSTRUCTION_TYPE_T;

/*
 * Last usable table number.
 * @OFPTT_MAX
 *
 * Fake tables.
 * @OFPTT_ALL           Wildcard table used for table config, flow stats and
 *                      flow deletes.
 */
typedef enum en_ofp_table
{
    EN_OFPTT_MAX    = 0xfe,
    EN_OFPTT_ALL    = 0xff,
}EN_OFP_TABLE_T;

/*
 * Flags to configure the table. Reserved for future use.
 *
 * @OFPTC_DEPRECATED_MASK       Deprecated bits
 */
typedef enum en_ofp_table_config
{
	EN_OFPTC_TABLE_MISS_CONTROLLER  = 0,		/* Send to controller. (1.0.0~1.2.0) */
	EN_OFPTC_TABLE_MISS_CONTINUE    = 1 << 0,	/* Continue to the next table in the
							 * pipeline (1.1.0~1.2.0) */
	EN_OFPTC_TABLE_MISS_DROP        = 1 << 1,	/* Drop the packet. (1.1.0~1.2.0) */
	EN_OFPTC_DEPRECATED_MASK        = 3,		/* (1.0.0~1.3.0) */
}EN_OFP_TABLE_CONFIG_T;

/* Table Feature property types.
 * Low order bit cleared indicates a property for a regular Flow Entry.
 * Low order bit set indicates a property for the Table-Miss Flow Entry.
 *
 * @OFPTFPT_INSTRUCTIONS                Instructions property.
 * @OFPTFPT_INSTRUCTIONS_MISS           Instructions for table-miss.
 * @OFPTFPT_NEXT_TABLES                 Next Table property.
 * @OFPTFPT_NEXT_TABLES_MISS            Next Table for table-miss.
 * @OFPTFPT_WRITE_ACTIONS               Write Actions property.
 * @OFPTFPT_WRITE_ACTIONS_MISS          Write Actions for table-miss.
 * @OFPTFPT_APPLY_ACTIONS               Apply Actions property.
 * @OFPTFPT_APPLY_ACTIONS_MISS          Apply Actions for table-miss.
 * @OFPTFPT_MATCH                       Match property.
 * @OFPTFPT_WILDCARDS                   Wildcards property.
 * @OFPTFPT_WRITE_SETFIELD              Write Set-Field property.
 * @OFPTFPT_WRITE_SETFIELD_MISS         Write Set-Field for table-miss.
 * @OFPTFPT_APPLY_SETFIELD              Apply Set-Field property.
 * @OFPTFPT_APPLY_SETFIELD_MISS         Apply Set-Field for table-miss.
 * @OFPTFPT_EXPERIMENTER                Experimenter property.
 * @OFPTFPT_EXPERIMENTER_MISS           Experimenter for table-miss.
 */
typedef enum en_ofp_table_feature_prop_type
{
	EN_OFPTFPT_INSTRUCTIONS         = 0,
	EN_OFPTFPT_INSTRUCTIONS_MISS    = 1,
	EN_OFPTFPT_NEXT_TABLES          = 2,
	EN_OFPTFPT_NEXT_TABLES_MISS     = 3,
	EN_OFPTFPT_WRITE_ACTIONS        = 4,
	EN_OFPTFPT_WRITE_ACTIONS_MISS   = 5,
	EN_OFPTFPT_APPLY_ACTIONS        = 6,
	EN_OFPTFPT_APPLY_ACTIONS_MISS   = 7,
	EN_OFPTFPT_MATCH                = 8,
	EN_OFPTFPT_WILDCARDS            = 10,
	EN_OFPTFPT_WRITE_SETFIELD       = 12,
	EN_OFPTFPT_WRITE_SETFIELD_MISS  = 13,
	EN_OFPTFPT_APPLY_SETFIELD       = 14,
	EN_OFPTFPT_APPLY_SETFIELD_MISS  = 15,
	EN_OFPTFPT_EXPERIMENTER         = 0xFFFE,
	EN_OFPTFPT_EXPERIMENTER_MISS    = 0xFFFF,
}EN_OFP_TABLE_FEAT_PROP_TYPE_T;

/* Why is this packet being sent to the controller?
 *
 * @OFPR_NO_MATCH           No matching flow (table-miss flow entry).
 * @OFPR_ACTION             Action explicitly output to controller.
 * @OFPR_INVALID_TTL        Packet has invalid TTL
 */
typedef enum en_ofp_packet_in_reason
{
    EN_OFPR_NO_MATCH        = 0,
    EN_OFPR_ACTION          = 1,
    EN_OFPR_INVALID_TTL     = 2,
}EN_OFP_PACKET_IN_REASON_T;

/*
 * @OFPFC_ADD                   New flow.
 * @OFPFC_MODIFY                Modify all matching flows.
 * @OFPFC_MODIFY_STRICT         Modify entry strictly matching wildcards
 *                              and priority.
 * @OFPFC_DELETE                Delete all matching flows.
 * @OFPFC_DELETE_STRICT         Delete entry strictly matching wildcards
 *                              and priority.
 */
typedef enum en_ofp_flow_mod_command
{
	EN_OFPFC_ADD            = 0,
	EN_OFPFC_MODIFY         = 1,
	EN_OFPFC_MODIFY_STRICT  = 2,
	EN_OFPFC_DELETE         = 3,
	EN_OFPFC_DELETE_STRICT  = 4,
}EN_OFP_FLOW_MOD_COMMAND_T;

/*
 * @OFPFF_SEND_FLOW_REM         Send flow removed message when flow expires
 *                              or is deleted.
 * @OFPFF_CHECK_OVERLAP         Check for overlapping entries first.
 * @OFPFF_RESET_COUNTS          Reset flow packet and byte counts.
 * @OFPFF_NO_PKT_COUNTS         Don't keep track of packet count.
 * @OFPFF_NO_BYT_COUNTS         Don't keep track of byte count.
 */
typedef enum en_ofp_flow_mod_flags
{
    EN_OFPFF_SEND_FLOW_REM      = 1 << 0,
    EN_OFPFF_CHECK_OVERLAP      = 1 << 1,
    EN_OFPFF_RESET_COUNTS       = 1 << 2,
    EN_OFPFF_NO_PKT_COUNTS      = 1 << 3,
    EN_OFPFF_NO_BYT_COUNTS      = 1 << 4,
}EN_OFP_FLOW_MOD_FLAGS_T;

/* Why was this flow removed?
 *
 * @OFPRR_IDLE_TIMEOUT      Flow idle time exceeded idle_timeout.
 * @OFPRR_HARD_TIMEOUT      Time exceeded hard_timeout.
 * @OFPRR_DELETE            Evicted by a DELETE flow mod.
 * @OFPRR_GROUP_DELETE      Group was removed.
 */
typedef enum en_ofp_flow_removed_reason
{
    EN_OFPRR_IDLE_TIMEOUT   = 0,
    EN_OFPRR_HARD_TIMEOUT   = 1,
    EN_OFPRR_DELETE         = 2,
    EN_OFPRR_GROUP_DELETE   = 3,
}EN_OFP_FLOW_REMOVED_REASON_T;

/* Group types. Values in the range [128, 255] are reserved for experimental use.
 *
 * @OFPGT_ALL           All (multicast/broadcast) group.
 * @OFPGT_SELECT        Select group.
 * @OFPGT_INDIRECT      Indirect group.
 * @OFPGT_FF            Fast failover group.
 */
typedef enum en_ofp_group_type
{
    EN_OFPGT_ALL        = 0,
    EN_OFPGT_SELECT     = 1,
    EN_OFPGT_INDIRECT   = 2,
    EN_OFPGT_FF         = 3,
}EN_OFP_GROUP_TYPE_T;

/* Group commands
 *
 * @OFPGC_ADD           New group.
 * @OFPGC_MODIFY        Modify all matching groups.
 * @OFPGC_DELETE        Delete all matching groups.
 */
typedef enum en_ofp_group_mod_command
{
    EN_OFPGC_ADD        = 0,
    EN_OFPGC_MODIFY     = 1,
    EN_OFPGC_DELETE     = 2,
}EN_OFP_GROUP_MOD_COMMAND_T;

/* Group configuration flags
 *
 * @OFPGFC_SELECT_WEIGHT        Support weight for select groups
 * @OFPGFC_SELECT_LIVENESS      Support liveness for select groups
 * @OFPGFC_CHAINING             Support chaining groups
 * @OFPGFC_CHAINING_CHECKS      Check chaining for loops and delete
 */
typedef enum en_ofp_group_capabilities
{
	EN_OFPGFC_SELECT_WEIGHT     = 1 << 0,
	EN_OFPGFC_SELECT_LIVENESS   = 1 << 1,
	EN_OFPGFC_CHAINING          = 1 << 2,
	EN_OFPGFC_CHAINING_CHECKS   = 1 << 3,
}EN_OFP_GROUP_CAPABILITIES_T;

/*
 * Last usable group number.
 * @OFPG_MAX
 *
 * Fake groups.
 * @OFPG_ALL        Represents all groups for group delete commands.
 * @OFPG_ANY        Wildcard group used only for flow stats requests.
 *                  Selects all flows regardless of group
 *                  (including flows with no group).
 */
typedef enum en_ofp_group
{
	EN_OFPG_MAX = 0xFFFFFF00,

	EN_OFPG_ALL = 0xFFFFFFFC,
	EN_OFPG_ANY = 0xFFFFFFFF,
}EN_OFP_GROUP_T;

/* Meter configuration flags
 *
 * @OFPMF_KBPS      Rate value in kb/s (kilo-bit per second).
 * @OFPMF_PKTPS     Rate value in packet/sec.
 * @OFPMF_BURST     Do burst size.
 * @OFPMF_STATS     Collect statistics.
 */
typedef enum en_ofp_meter_flags
{
	EN_OFPMF_KBPS   = 1 << 0,
	EN_OFPMF_PKTPS  = 1 << 1,
	EN_OFPMF_BURST  = 1 << 2,
	EN_OFPMF_STATS  = 1 << 3,
}EN_OFP_METER_FLAGS_T;

/* Meter numbering. Flow meters can use any number up to OFPM_MAX.
 *
 * Last usable meter.
 * @OFPM_MAX
 *
 * Virtual meters.
 * @OFPM_SLOWPATH       Meter for slow datapath.
 * @OFPM_CONTROLLER     Meter for controller connection.
 * @OFPM_ALL            Represents all meters for stat requests commands.
 */
typedef enum en_ofp_meter
{
	EN_OFPM_MAX             = 0xFFFF0000,

	EN_OFPM_SLOWPATH        = 0xFFFFFFFD,
	EN_OFPM_CONTROLLER      = 0xFFFFFFFE,
	EN_OFPM_ALL             = 0xFFFFFFFF,
}EN_OFP_METER_T;

/*
 * @OFPMC_ADD           New meter.
 * @OFPMC_MODIFY        Modify specified meter.
 * @OFPMC_DELETE        Delete specified meter.
 */
typedef enum en_ofp_meter_mod_command
{
	EN_OFPMC_ADD,
	EN_OFPMC_MODIFY,
	EN_OFPMC_DELETE,
}EN_OFP_METER_MOD_COMMAND_T;

/* Meter band types
 *
 * @OFPMBT_DROP             Drop packet.
 * @OFPMBT_DSCP_REMARK      Remark DSCP in the IP header.
 * @OFPMBT_EXPERIMENTER     Experimenter meter band.
 */
typedef enum en_ofp_meter_band_type
{
	EN_OFPMBT_DROP          = 1,
	EN_OFPMBT_DSCP_REMARK   = 2,
	EN_OFPMBT_EXPERIMENTER  = 0xFFFF,
}EN_OFP_METER_BAND_TYPE_T;

/*
 * @OFPCML_MAX          maximum max_len value which can be used to request a
 *                      specific byte length.
 * @OFPCML_NO_BUFFER    indicates that no buffering should be applied and the
 *                      whole packet is to be sent to the controller.
 */
typedef enum en_ofp_controller_max_len
{
	EN_OFPCML_MAX			= 0xFFE5,
	EN_OFPCML_NO_BUFFER		= 0xFFFF,
}EN_OFP_CONTROLLER_MAX_LEN_T;


#endif /* __OFP_COMMON_ENUM_H__ */

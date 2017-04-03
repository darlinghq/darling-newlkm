/*
Copyright (c) 2014-2017, Wenqi Chen

Shanghai Mifu Infotech Co., Ltd
B112-113, IF Industrial Park, 508 Chunking Road, Shanghai 201103, China


All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

#ifndef DUCT__PRE_XNU_SOCKETS_H
#define DUCT__PRE_XNU_SOCKETS_H

// linux/include/include/linux/socket.h
#undef AF_UNSPEC
#undef AF_UNIX
#undef AF_LOCAL
#undef AF_INET
#undef AF_AX25
#undef AF_IPX
#undef AF_APPLETALK
#undef AF_NETROM
#undef AF_BRIDGE
#undef AF_ATMPVC
#undef AF_X25
#undef AF_INET6
#undef AF_ROSE
#undef AF_DECnet
#undef AF_NETBEUI
#undef AF_SECURITY
#undef AF_KEY
#undef AF_NETLINK
#undef AF_ROUTE
#undef AF_PACKET
#undef AF_ASH
#undef AF_ECONET
#undef AF_ATMSVC
#undef AF_RDS
#undef AF_SNA
#undef AF_IRDA
#undef AF_PPPOX
#undef AF_WANPIPE
#undef AF_LLC
#undef AF_CAN
#undef AF_TIPC
#undef AF_BLUETOOTH
#undef AF_IUCV
#undef AF_RXRPC
#undef AF_ISDN
#undef AF_PHONET
#undef AF_IEEE802154
#undef AF_CAIF
#undef AF_ALG
#undef AF_NFC
#undef AF_MAX

#define LINUX_AF_UNSPEC         0
#define LINUX_AF_UNIX           1    /* Unix domain sockets         */
#define LINUX_AF_LOCAL          1    /* POSIX name for AF_UNIX    */
#define LINUX_AF_INET           2    /* Internet IP Protocol     */
#define LINUX_AF_AX25           3    /* Amateur Radio AX.25         */
#define LINUX_AF_IPX            4    /* Novell IPX             */
#define LINUX_AF_APPLETALK      5    /* AppleTalk DDP         */
#define LINUX_AF_NETROM         6    /* Amateur Radio NET/ROM     */
#define LINUX_AF_BRIDGE         7    /* Multiprotocol bridge     */
#define LINUX_AF_ATMPVC         8    /* ATM PVCs            */
#define LINUX_AF_X25            9    /* Reserved for X.25 project     */
#define LINUX_AF_INET6          10    /* IP version 6            */
#define LINUX_AF_ROSE           11    /* Amateur Radio X.25 PLP    */
#define LINUX_AF_DECnet         12    /* Reserved for DECnet project    */
#define LINUX_AF_NETBEUI        13    /* Reserved for 802.2LLC project*/
#define LINUX_AF_SECURITY       14    /* Security callback pseudo AF */
#define LINUX_AF_KEY            15      /* PF_KEY key management API */
#define LINUX_AF_NETLINK        16
#define LINUX_AF_ROUTE          LINUX_AF_NETLINK /* Alias to emulate 4.4BSD */
#define LINUX_AF_PACKET         17    /* Packet family        */
#define LINUX_AF_ASH            18    /* Ash                */
#define LINUX_AF_ECONET         19    /* Acorn Econet            */
#define LINUX_AF_ATMSVC         20    /* ATM SVCs            */
#define LINUX_AF_RDS            21    /* RDS sockets             */
#define LINUX_AF_SNA            22    /* Linux SNA Project (nutters!) */
#define LINUX_AF_IRDA           23    /* IRDA sockets            */
#define LINUX_AF_PPPOX          24    /* PPPoX sockets        */
#define LINUX_AF_WANPIPE        25    /* Wanpipe API Sockets */
#define LINUX_AF_LLC            26    /* Linux LLC            */
#define LINUX_AF_CAN            29    /* Controller Area Network      */
#define LINUX_AF_TIPC           30    /* TIPC sockets            */
#define LINUX_AF_BLUETOOTH      31    /* Bluetooth sockets         */
#define LINUX_AF_IUCV           32    /* IUCV sockets            */
#define LINUX_AF_RXRPC          33    /* RxRPC sockets         */
#define LINUX_AF_ISDN           34    /* mISDN sockets         */
#define LINUX_AF_PHONET         35    /* Phonet sockets        */
#define LINUX_AF_IEEE802154     36    /* IEEE802154 sockets        */
#define LINUX_AF_CAIF           37    /* CAIF sockets            */
#define LINUX_AF_ALG            38    /* Algorithm sockets        */
#define LINUX_AF_NFC            39    /* NFC sockets            */
#define LINUX_AF_MAX            40    /* For now.. */



// linux/include/include/linux/socket.h
#undef PF_UNSPEC
#undef PF_UNIX
#undef PF_LOCAL
#undef PF_INET
#undef PF_AX25
#undef PF_IPX
#undef PF_APPLETALK
#undef PF_NETROM
#undef PF_BRIDGE
#undef PF_ATMPVC
#undef PF_X25
#undef PF_INET6
#undef PF_ROSE
#undef PF_DECnet
#undef PF_NETBEUI
#undef PF_SECURITY
#undef PF_KEY
#undef PF_NETLINK
#undef PF_ROUTE
#undef PF_PACKET
#undef PF_ASH
#undef PF_ECONET
#undef PF_ATMSVC
#undef PF_RDS
#undef PF_SNA
#undef PF_IRDA
#undef PF_PPPOX
#undef PF_WANPIPE
#undef PF_LLC
#undef PF_CAN
#undef PF_TIPC
#undef PF_BLUETOOTH
#undef PF_IUCV
#undef PF_RXRPC
#undef PF_ISDN
#undef PF_PHONET
#undef PF_IEEE802154
#undef PF_CAIF
#undef PF_ALG
#undef PF_NFC
#undef PF_MAX

#define LINUX_PF_UNSPEC         LINUX_AF_UNSPEC
#define LINUX_PF_UNIX           LINUX_AF_UNIX
#define LINUX_PF_LOCAL          LINUX_AF_LOCAL
#define LINUX_PF_INET           LINUX_AF_INET
#define LINUX_PF_AX25           LINUX_AF_AX25
#define LINUX_PF_IPX            LINUX_AF_IPX
#define LINUX_PF_APPLETALK      LINUX_AF_APPLETALK
#define LINUX_PF_NETROM         LINUX_AF_NETROM
#define LINUX_PF_BRIDGE         LINUX_AF_BRIDGE
#define LINUX_PF_ATMPVC         LINUX_AF_ATMPVC
#define LINUX_PF_X25            LINUX_AF_X25
#define LINUX_PF_INET6          LINUX_AF_INET6
#define LINUX_PF_ROSE           LINUX_AF_ROSE
#define LINUX_PF_DECnet         LINUX_AF_DECnet
#define LINUX_PF_NETBEUI        LINUX_AF_NETBEUI
#define LINUX_PF_SECURITY       LINUX_AF_SECURITY
#define LINUX_PF_KEY            LINUX_AF_KEY
#define LINUX_PF_NETLINK        LINUX_AF_NETLINK
#define LINUX_PF_ROUTE          LINUX_AF_ROUTE
#define LINUX_PF_PACKET         LINUX_AF_PACKET
#define LINUX_PF_ASH            LINUX_AF_ASH
#define LINUX_PF_ECONET         LINUX_AF_ECONET
#define LINUX_PF_ATMSVC         LINUX_AF_ATMSVC
#define LINUX_PF_RDS            LINUX_AF_RDS
#define LINUX_PF_SNA            LINUX_AF_SNA
#define LINUX_PF_IRDA           LINUX_AF_IRDA
#define LINUX_PF_PPPOX          LINUX_AF_PPPOX
#define LINUX_PF_WANPIPE        LINUX_AF_WANPIPE
#define LINUX_PF_LLC            LINUX_AF_LLC
#define LINUX_PF_CAN            LINUX_AF_CAN
#define LINUX_PF_TIPC           LINUX_AF_TIPC
#define LINUX_PF_BLUETOOTH      LINUX_AF_BLUETOOTH
#define LINUX_PF_IUCV           LINUX_AF_IUCV
#define LINUX_PF_RXRPC          LINUX_AF_RXRPC
#define LINUX_PF_ISDN           LINUX_AF_ISDN
#define LINUX_PF_PHONET         LINUX_AF_PHONET
#define LINUX_PF_IEEE802154     LINUX_AF_IEEE802154
#define LINUX_PF_CAIF           LINUX_AF_CAIF
#define LINUX_PF_ALG            LINUX_AF_ALG
#define LINUX_PF_NFC            LINUX_AF_NFC
#define LINUX_PF_MAX            LINUX_AF_MAX


// linux/include/include/linux/socket.h
#undef SOMAXCONN
#define LINUX_SOMAXCONN         128


// linux/include/include/linux/socket.h
#undef MSG_OOB
#undef MSG_PEEK
#undef MSG_DONTROUTE
#undef MSG_TRYHARD
#undef MSG_CTRUNC
#undef MSG_PROBE
#undef MSG_TRUNC
#undef MSG_DONTWAIT
#undef MSG_EOR
#undef MSG_WAITALL
#undef MSG_FIN
#undef MSG_SYN
#undef MSG_CONFIRM
#undef MSG_RST
#undef MSG_ERRQUEUE
#undef MSG_NOSIGNAL
#undef MSG_MORE
#undef MSG_WAITFORONE
#undef MSG_SENDPAGE_NOTLAST
#undef MSG_EOF
#undef MSG_CMSG_CLOEXEC
#undef MSG_CMSG_COMPAT

#define LINUX_MSG_OOB               1
#define LINUX_MSG_PEEK              2
#define LINUX_MSG_DONTROUTE         4
#define LINUX_MSG_TRYHARD           4               /* Synonym for MSG_DONTROUTE for DECnet */
#define LINUX_MSG_CTRUNC            8
#define LINUX_MSG_PROBE             0x10            /* Do not send. Only probe path f.e. for MTU */
#define LINUX_MSG_TRUNC             0x20
#define LINUX_MSG_DONTWAIT          0x40            /* Nonblocking io         */
#define LINUX_MSG_EOR               0x80            /* End of record */
#define LINUX_MSG_WAITALL           0x100           /* Wait for a full request */
#define LINUX_MSG_FIN               0x200
#define LINUX_MSG_SYN               0x400
#define LINUX_MSG_CONFIRM           0x800           /* Confirm path validity */
#define LINUX_MSG_RST               0x1000
#define LINUX_MSG_ERRQUEUE          0x2000          /* Fetch message from error queue */
#define LINUX_MSG_NOSIGNAL          0x4000          /* Do not generate SIGPIPE */
#define LINUX_MSG_MORE              0x8000          /* Sender will send more */
#define LINUX_MSG_WAITFORONE        0x10000         /* recvmmsg(): block until 1+ packets avail */
#define LINUX_MSG_SENDPAGE_NOTLAST  0x20000         /* sendpage() internal : not the last page */
#define LINUX_MSG_EOF               LINUX_MSG_FIN

#define LINUX_MSG_CMSG_CLOEXEC      0x40000000      /* Set close_on_exit for file */

#if defined(CONFIG_COMPAT)
#define LINUX_MSG_CMSG_COMPAT       0x80000000      /* This message needs 32 bit fixups */
#else
#define LINUX_MSG_CMSG_COMPAT       0               /* We never have 32 bit fixups */
#endif


// linux/include/include/linux/socket.h
#undef __CMSG_NXTHDR
#undef CMSG_NXTHDR
#undef CMSG_ALIGN
#undef CMSG_DATA
#undef CMSG_SPACE
#undef CMSG_LEN
#undef __CMSG_FIRSTHDR
#undef CMSG_FIRSTHDR
#undef CMSG_OK


// linux/arch/arm/include/asm/socket.h  
#undef SOL_SOCKET

#undef SO_DEBUG
#undef SO_REUSEADDR
#undef SO_TYPE
#undef SO_ERROR
#undef SO_DONTROUTE
#undef SO_BROADCAST
#undef SO_SNDBUF
#undef SO_RCVBUF
#undef SO_SNDBUFFORCE
#undef SO_RCVBUFFORCE
#undef SO_KEEPALIVE
#undef SO_OOBINLINE
#undef SO_NO_CHECK
#undef SO_PRIORITY
#undef SO_LINGER
#undef SO_BSDCOMPAT

#undef SO_PASSCRED
#undef SO_PEERCRED
#undef SO_RCVLOWAT
#undef SO_SNDLOWAT
#undef SO_RCVTIMEO
#undef SO_SNDTIMEO

#undef SO_SECURITY_AUTHENTICATION
#undef SO_SECURITY_ENCRYPTION_TRANSPORT
#undef SO_SECURITY_ENCRYPTION_NETWORK

#undef SO_BINDTODEVICE

#undef SO_ATTACH_FILTER
#undef SO_DETACH_FILTER

#undef SO_PEERNAME
#undef SO_TIMESTAMP
#undef SCM_TIMESTAMP

#undef SO_ACCEPTCONN

#undef SO_PEERSEC
#undef SO_PASSSEC
#undef SO_TIMESTAMPNS
#undef SCM_TIMESTAMPNS

#undef SO_MARK

#undef SO_TIMESTAMPING
#undef SCM_TIMESTAMPING

#undef SO_PROTOCOL
#undef SO_DOMAIN

#undef SO_RXQ_OVFL

#undef SO_WIFI_STATUS
#undef SCM_WIFI_STATUS
#undef SO_PEEK_OFF

#undef SO_NOFCS


#define LINUX_SOL_SOCKET        1

#define LINUX_SO_DEBUG          1
#define LINUX_SO_REUSEADDR      2
#define LINUX_SO_TYPE           3
#define LINUX_SO_ERROR          4
#define LINUX_SO_DONTROUTE      5
#define LINUX_SO_BROADCAST      6
#define LINUX_SO_SNDBUF         7
#define LINUX_SO_RCVBUF         8
#define LINUX_SO_SNDBUFFORCE    32
#define LINUX_SO_RCVBUFFORCE    33
#define LINUX_SO_KEEPALIVE      9
#define LINUX_SO_OOBINLINE      10
#define LINUX_SO_NO_CHECK       11
#define LINUX_SO_PRIORITY       12
#define LINUX_SO_LINGER         13
#define LINUX_SO_BSDCOMPAT      14

#define LINUX_SO_PASSCRED       16
#define LINUX_SO_PEERCRED       17
#define LINUX_SO_RCVLOWAT       18
#define LINUX_SO_SNDLOWAT       19
#define LINUX_SO_RCVTIMEO       20
#define LINUX_SO_SNDTIMEO       21

#define LINUX_SO_SECURITY_AUTHENTICATION            22
#define LINUX_SO_SECURITY_ENCRYPTION_TRANSPORT      23
#define LINUX_SO_SECURITY_ENCRYPTION_NETWORK        24

#define LINUX_SO_BINDTODEVICE       25

#define LINUX_SO_ATTACH_FILTER      26
#define LINUX_SO_DETACH_FILTER      27

#define LINUX_SO_PEERNAME           28
#define LINUX_SO_TIMESTAMP          29
#define LINUX_SCM_TIMESTAMP         LINUX_SO_TIMESTAMP

#define LINUX_SO_ACCEPTCONN         30

#define LINUX_SO_PEERSEC            31
#define LINUX_SO_PASSSEC            34
#define LINUX_SO_TIMESTAMPNS        35
#define LINUX_SCM_TIMESTAMPNS       LINUX_SO_TIMESTAMPNS

#define LINUX_SO_MARK               36

#define LINUX_SO_TIMESTAMPING       37
#define LINUX_SCM_TIMESTAMPING      LINUX_SO_TIMESTAMPING

#define LINUX_SO_PROTOCOL           38
#define LINUX_SO_DOMAIN             39

#define LINUX_SO_RXQ_OVFL           40

#define LINUX_SO_WIFI_STATUS        41
#define LINUX_SCM_WIFI_STATUS       LINUX_SO_WIFI_STATUS
#define LINUX_SO_PEEK_OFF           42

#define LINUX_SO_NOFCS              43


#endif // DUCT__PRE_XNU_SOCKETS_H

                       

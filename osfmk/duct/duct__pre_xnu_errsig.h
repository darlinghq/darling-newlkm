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

#ifndef DUCT__PRE_XNU_ERRSIG_H
#define DUCT__PRE_XNU_ERRSIG_H

// linux/include/asm-generic/errno-base.h
#define LINUX_EPERM             1    /* Operation not permitted */
#define LINUX_ENOENT            2    /* No such file or directory */
#define LINUX_ESRCH             3    /* No such process */
#define LINUX_EINTR             4    /* Interrupted system call */
#define LINUX_EIO               5    /* I/O error */
#define LINUX_ENXIO             6    /* No such device or address */
#define LINUX_E2BIG             7    /* Argument list too long */
#define LINUX_ENOEXEC           8    /* Exec format error */
#define LINUX_EBADF             9    /* Bad file number */
#define LINUX_ECHILD            10    /* No child processes */
#define LINUX_EAGAIN            11    /* Try again */
#define LINUX_ENOMEM            12    /* Out of memory */
#define LINUX_EACCES            13    /* Permission denied */
#define LINUX_EFAULT            14    /* Bad address */
#define LINUX_ENOTBLK           15    /* Block device required */
#define LINUX_EBUSY             16    /* Device or resource busy */
#define LINUX_EEXIST            17    /* File exists */
#define LINUX_EXDEV             18    /* Cross-device link */
#define LINUX_ENODEV            19    /* No such device */
#define LINUX_ENOTDIR           20    /* Not a directory */
#define LINUX_EISDIR            21    /* Is a directory */
#define LINUX_EINVAL            22    /* Invalid argument */
#define LINUX_ENFILE            23    /* File table overflow */
#define LINUX_EMFILE            24    /* Too many open files */
#define LINUX_ENOTTY            25    /* Not a typewriter */
#define LINUX_ETXTBSY           26    /* Text file busy */
#define LINUX_EFBIG             27    /* File too large */
#define LINUX_ENOSPC            28    /* No space left on device */
#define LINUX_ESPIPE            29    /* Illegal seek */
#define LINUX_EROFS             30    /* Read-only file system */
#define LINUX_EMLINK            31    /* Too many links */
#define LINUX_EPIPE             32    /* Broken pipe */
#define LINUX_EDOM              33    /* Math argument out of domain of func */
#define LINUX_ERANGE            34    /* Math result not representable */

// linux/include/asm-generic/errno.h
#define LINUX_EDEADLK           35    /* Resource deadlock would occur */
#define LINUX_ENAMETOOLONG      36    /* File name too long */
#define LINUX_ENOLCK            37    /* No record locks available */
#define LINUX_ENOSYS            38    /* Function not implemented */
#define LINUX_ENOTEMPTY         39    /* Directory not empty */
#define LINUX_ELOOP             40    /* Too many symbolic links encountered */
#define LINUX_EWOULDBLOCK       LINUX_EAGAIN    /* Operation would block */
#define LINUX_ENOMSG            42    /* No message of desired type */
#define LINUX_EIDRM             43    /* Identifier removed */
#define LINUX_ECHRNG            44    /* Channel number out of range */
#define LINUX_EL2NSYNC          45    /* Level 2 not synchronized */
#define LINUX_EL3HLT            46    /* Level 3 halted */
#define LINUX_EL3RST            47    /* Level 3 reset */
#define LINUX_ELNRNG            48    /* Link number out of range */
#define LINUX_EUNATCH           49    /* Protocol driver not attached */
#define LINUX_ENOCSI            50    /* No CSI structure available */
#define LINUX_EL2HLT            51    /* Level 2 halted */
#define LINUX_EBADE             52    /* Invalid exchange */
#define LINUX_EBADR             53    /* Invalid request descriptor */
#define LINUX_EXFULL            54    /* Exchange full */
#define LINUX_ENOANO            55    /* No anode */
#define LINUX_EBADRQC           56    /* Invalid request code */
#define LINUX_EBADSLT           57    /* Invalid slot */

#define LINUX_EDEADLOCK         LINUX_EDEADLK

#define LINUX_EBFONT            59    /* Bad font file format */
#define LINUX_ENOSTR            60    /* Device not a stream */
#define LINUX_ENODATA           61    /* No data available */
#define LINUX_ETIME             62    /* Timer expired */
#define LINUX_ENOSR             63    /* Out of streams resources */
#define LINUX_ENONET            64    /* Machine is not on the network */
#define LINUX_ENOPKG            65    /* Package not installed */
#define LINUX_EREMOTE           66    /* Object is remote */
#define LINUX_ENOLINK           67    /* Link has been severed */
#define LINUX_EADV              68    /* Advertise error */
#define LINUX_ESRMNT            69    /* Srmount error */
#define LINUX_ECOMM             70    /* Communication error on send */
#define LINUX_EPROTO            71    /* Protocol error */
#define LINUX_EMULTIHOP         72    /* Multihop attempted */
#define LINUX_EDOTDOT           73    /* RFS specific error */
#define LINUX_EBADMSG           74    /* Not a data message */
#define LINUX_EOVERFLOW         75    /* Value too large for defined data type */
#define LINUX_ENOTUNIQ          76    /* Name not unique on network */
#define LINUX_EBADFD            77    /* File descriptor in bad state */
#define LINUX_EREMCHG           78    /* Remote address changed */
#define LINUX_ELIBACC           79    /* Can not access a needed shared library */
#define LINUX_ELIBBAD           80    /* Accessing a corrupted shared library */
#define LINUX_ELIBSCN           81    /* .lib section in a.out corrupted */
#define LINUX_ELIBMAX           82    /* Attempting to link in too many shared libraries */
#define LINUX_ELIBEXEC          83    /* Cannot exec a shared library directly */
#define LINUX_EILSEQ            84    /* Illegal byte sequence */
#define LINUX_ERESTART          85    /* Interrupted system call should be restarted */
#define LINUX_ESTRPIPE          86    /* Streams pipe error */
#define LINUX_EUSERS            87    /* Too many users */
#define LINUX_ENOTSOCK          88    /* Socket operation on non-socket */
#define LINUX_EDESTADDRREQ      89    /* Destination address required */
#define LINUX_EMSGSIZE          90    /* Message too long */
#define LINUX_EPROTOTYPE        91    /* Protocol wrong type for socket */
#define LINUX_ENOPROTOOPT       92    /* Protocol not available */
#define LINUX_EPROTONOSUPPORT   93    /* Protocol not supported */
#define LINUX_ESOCKTNOSUPPORT   94    /* Socket type not supported */
#define LINUX_EOPNOTSUPP        95    /* Operation not supported on transport endpoint */
#define LINUX_EPFNOSUPPORT      96    /* Protocol family not supported */
#define LINUX_EAFNOSUPPORT      97    /* Address family not supported by protocol */
#define LINUX_EADDRINUSE        98    /* Address already in use */
#define LINUX_EADDRNOTAVAIL     99    /* Cannot assign requested address */
#define LINUX_ENETDOWN          100    /* Network is down */
#define LINUX_ENETUNREACH       101    /* Network is unreachable */
#define LINUX_ENETRESET         102    /* Network dropped connection because of reset */
#define LINUX_ECONNABORTED      103    /* Software caused connection abort */
#define LINUX_ECONNRESET        104    /* Connection reset by peer */
#define LINUX_ENOBUFS           105    /* No buffer space available */
#define LINUX_EISCONN           106    /* Transport endpoint is already connected */
#define LINUX_ENOTCONN          107    /* Transport endpoint is not connected */
#define LINUX_ESHUTDOWN         108    /* Cannot send after transport endpoint shutdown */
#define LINUX_ETOOMANYREFS      109    /* Too many references: cannot splice */
#define LINUX_ETIMEDOUT         110    /* Connection timed out */
#define LINUX_ECONNREFUSED      111    /* Connection refused */
#define LINUX_EHOSTDOWN         112    /* Host is down */
#define LINUX_EHOSTUNREACH      113    /* No route to host */
#define LINUX_EALREADY          114    /* Operation already in progress */
#define LINUX_EINPROGRESS       115    /* Operation now in progress */
#define LINUX_ESTALE            116    /* Stale NFS file handle */
#define LINUX_EUCLEAN           117    /* Structure needs cleaning */
#define LINUX_ENOTNAM           118    /* Not a XENIX named type file */
#define LINUX_ENAVAIL           119    /* No XENIX semaphores available */
#define LINUX_EISNAM            120    /* Is a named type file */
#define LINUX_EREMOTEIO         121    /* Remote I/O error */
#define LINUX_EDQUOT            122    /* Quota exceeded */

#define LINUX_ENOMEDIUM         123    /* No medium found */
#define LINUX_EMEDIUMTYPE       124    /* Wrong medium type */
#define LINUX_ECANCELED         125    /* Operation Canceled */
#define LINUX_ENOKEY            126    /* Required key not available */
#define LINUX_EKEYEXPIRED       127    /* Key has expired */
#define LINUX_EKEYREVOKED       128    /* Key has been revoked */
#define LINUX_EKEYREJECTED      129    /* Key was rejected by service */

#define LINUX_EOWNERDEAD        130    /* Owner died */
#define LINUX_ENOTRECOVERABLE   131    /* State not recoverable */

#define LINUX_ERFKILL           132    /* Operation not possible due to RF-kill */

#define LINUX_EHWPOISON         133    /* Memory page has hardware error */

// linux/include/errno.h
#define LINUX_ERESTARTSYS               512
#define LINUX_ERESTARTNOINTR            513
#define LINUX_ERESTARTNOHAND            514    /* restart if no handler.. */
#define LINUX_ENOIOCTLCMD               515    /* No ioctl command */
#define LINUX_ERESTART_RESTARTBLOCK     516 /* restart by calling sys_restart_syscall */
#define LINUX_EPROBE_DEFER              517    /* Driver requests probe retry */

#define LINUX_EBADHANDLE                521    /* Illegal NFS file handle */
#define LINUX_ENOTSYNC                  522    /* Update synchronization mismatch */
#define LINUX_EBADCOOKIE                523    /* Cookie is stale */
#define LINUX_ENOTSUPP                  524    /* Operation is not supported */
#define LINUX_ETOOSMALL                 525    /* Buffer or request is too small */
#define LINUX_ESERVERFAULT              526    /* An untranslatable error occurred */
#define LINUX_EBADTYPE                  527    /* Type not supported by server */
#define LINUX_EJUKEBOX                  528    /* Request initiated, but will not complete before timeout */
#define LINUX_EIOCBQUEUED               529    /* iocb queued, will get completion event */
#define LINUX_EIOCBRETRY                530    /* iocb queued, will trigger a retry */


#undef EPERM
#undef ENOENT
#undef ESRCH
#undef EINTR
#undef EIO
#undef ENXIO
#undef E2BIG
#undef ENOEXEC
#undef EBADF
#undef ECHILD
#undef EAGAIN
#undef ENOMEM
#undef EACCES
#undef EFAULT
#undef ENOTBLK
#undef EBUSY
#undef EEXIST
#undef EXDEV
#undef ENODEV
#undef ENOTDIR
#undef EISDIR
#undef EINVAL
#undef ENFILE
#undef EMFILE
#undef ENOTTY
#undef ETXTBSY
#undef EFBIG
#undef ENOSPC
#undef ESPIPE
#undef EROFS
#undef EMLINK
#undef EPIPE
#undef EDOM
#undef ERANGE

#undef EDEADLK
#undef ENAMETOOLONG
#undef ENOLCK
#undef ENOSYS
#undef ENOTEMPTY
#undef ELOOP
#undef EWOULDBLOCK
#undef ENOMSG
#undef EIDRM
#undef ECHRNG
#undef EL2NSYNC
#undef EL3HLT
#undef EL3RST
#undef ELNRNG
#undef EUNATCH
#undef ENOCSI
#undef EL2HLT
#undef EBADE
#undef EBADR
#undef EXFULL
#undef ENOANO
#undef EBADRQC
#undef EBADSLT

#undef EDEADLOCK

#undef EBFONT
#undef ENOSTR
#undef ENODATA
#undef ETIME
#undef ENOSR
#undef ENONET
#undef ENOPKG
#undef EREMOTE
#undef ENOLINK
#undef EADV
#undef ESRMNT
#undef ECOMM
#undef EPROTO
#undef EMULTIHOP
#undef EDOTDOT
#undef EBADMSG
#undef EOVERFLOW
#undef ENOTUNIQ
#undef EBADFD
#undef EREMCHG
#undef ELIBACC
#undef ELIBBAD
#undef ELIBSCN
#undef ELIBMAX
#undef ELIBEXEC
#undef EILSEQ
#undef ERESTART
#undef ESTRPIPE
#undef EUSERS
#undef ENOTSOCK
#undef EDESTADDRREQ
#undef EMSGSIZE
#undef EPROTOTYPE
#undef ENOPROTOOPT
#undef EPROTONOSUPPORT
#undef ESOCKTNOSUPPORT
#undef EOPNOTSUPP
#undef EPFNOSUPPORT
#undef EAFNOSUPPORT
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef ENETDOWN
#undef ENETUNREACH
#undef ENETRESET
#undef ECONNABORTED
#undef ECONNRESET
#undef ENOBUFS
#undef EISCONN
#undef ENOTCONN
#undef ESHUTDOWN
#undef ETOOMANYREFS
#undef ETIMEDOUT
#undef ECONNREFUSED
#undef EHOSTDOWN
#undef EHOSTUNREACH
#undef EALREADY
#undef EINPROGRESS
#undef ESTALE
#undef EUCLEAN
#undef ENOTNAM
#undef ENAVAIL
#undef EISNAM
#undef EREMOTEIO
#undef EDQUOT

#undef ENOMEDIUM
#undef EMEDIUMTYPE
#undef ECANCELED
#undef ENOKEY
#undef EKEYEXPIRED
#undef EKEYREVOKED
#undef EKEYREJECTED

/* for robust mutexes */
#undef EOWNERDEAD
#undef ENOTRECOVERABLE

#undef ERFKILL

#undef EHWPOISON

#undef ERESTARTSYS
#undef ERESTARTNOINTR
#undef ERESTARTNOHAND
#undef ENOIOCTLCMD
#undef ERESTART_RESTARTBLOCK
#undef EPROBE_DEFER

#undef EBADHANDLE
#undef ENOTSYNC
#undef EBADCOOKIE
#undef ENOTSUPP
#undef ETOOSMALL
#undef ESERVERFAULT
#undef EBADTYPE
#undef EJUKEBOX
#undef EIOCBQUEUED
#undef EIOCBRETRY

// include/asm-generic/signal.h
#define LINUX_SIGHUP            1
#define LINUX_SIGINT            2
#define LINUX_SIGQUIT           3
#define LINUX_SIGILL            4
#define LINUX_SIGTRAP           5
#define LINUX_SIGABRT           6
#define LINUX_SIGIOT            6
#define LINUX_SIGBUS            7
#define LINUX_SIGFPE            8
#define LINUX_SIGKILL           9
#define LINUX_SIGUSR1           10
#define LINUX_SIGSEGV           11
#define LINUX_SIGUSR2           12
#define LINUX_SIGPIPE           13
#define LINUX_SIGALRM           14
#define LINUX_SIGTERM           15
#define LINUX_SIGSTKFLT         16
#define LINUX_SIGCHLD           17
#define LINUX_SIGCONT           18
#define LINUX_SIGSTOP           19
#define LINUX_SIGTSTP           20
#define LINUX_SIGTTIN           21
#define LINUX_SIGTTOU           22
#define LINUX_SIGURG            23
#define LINUX_SIGXCPU           24
#define LINUX_SIGXFSZ           25
#define LINUX_SIGVTALRM         26
#define LINUX_SIGPROF           27
#define LINUX_SIGWINCH          28
#define LINUX_SIGIO             29
#define LINUX_SIGPOLL           LINUX_SIGIO
#define LINUX_SIGPWR            30
#define LINUX_SIGSYS            31
#define LINUX_SIGUNUSED         31

#define	XNU_SIGHUP	1
#define	XNU_SIGINT	2
#define	XNU_SIGQUIT	3
#define	XNU_SIGILL	4
#define	XNU_SIGTRAP	5
#define	XNU_SIGABRT	6
#define	XNU_SIGPOLL	7
#define	XNU_SIGFPE	8
#define	XNU_SIGKILL	9
#define	XNU_SIGBUS	10
#define	XNU_SIGSEGV	11
#define	XNU_SIGSYS	12
#define	XNU_SIGPIPE	13
#define	XNU_SIGALRM	14
#define	XNU_SIGTERM	15
#define	XNU_SIGURG	16
#define	XNU_SIGSTOP	17
#define	XNU_SIGTSTP	18
#define	XNU_SIGCONT	19
#define	XNU_SIGCHLD	20
#define	XNU_SIGTTIN	21
#define	XNU_SIGTTOU	22
#define	XNU_SIGIO	23
#define	XNU_SIGXCPU	24
#define	XNU_SIGXFSZ	25
#define	XNU_SIGVTALRM 26
#define	XNU_SIGPROF	27
#define XNU_SIGWINCH 28
#define XNU_SIGINFO	29
#define XNU_SIGUSR1 30
#define XNU_SIGUSR2 31

#undef SIGHUP
#undef SIGINT
#undef SIGQUIT
#undef SIGILL
#undef SIGTRAP
#undef SIGABRT
#undef SIGIOT
#undef SIGBUS
#undef SIGFPE
#undef SIGKILL
#undef SIGUSR1
#undef SIGSEGV
#undef SIGUSR2
#undef SIGPIPE
#undef SIGALRM
#undef SIGTERM
#undef SIGSTKFLT
#undef SIGCHLD
#undef SIGCONT
#undef SIGSTOP
#undef SIGTSTP
#undef SIGTTIN
#undef SIGTTOU
#undef SIGURG
#undef SIGXCPU
#undef SIGXFSZ
#undef SIGVTALRM
#undef SIGPROF
#undef SIGWINCH
#undef SIGIO
#undef SIGPOLL
#undef SIGPWR
#undef SIGSYS
#undef SIGUNUSED

#endif // DUCT__PRE_XNU_ERRSIG_H

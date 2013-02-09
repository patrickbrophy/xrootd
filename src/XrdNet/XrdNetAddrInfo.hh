#ifndef __XRDNETADDRINFO_HH__
#define __XRDNETADDRINFO_HH__
/******************************************************************************/
/*                                                                            */
/*                     X r d N e t A d d r I n f o . h h                      */
/*                                                                            */
/* (c) 2013 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/*                                                                            */
/* This file is part of the XRootD software suite.                            */
/*                                                                            */
/* XRootD is free software: you can redistribute it and/or modify it under    */
/* the terms of the GNU Lesser General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or (at your     */
/* option) any later version.                                                 */
/*                                                                            */
/* XRootD is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public       */
/* License for more details.                                                  */
/*                                                                            */
/* You should have received a copy of the GNU Lesser General Public License   */
/* along with XRootD in a file called COPYING.LESSER (LGPL license) and file  */
/* COPYING (GPL license).  If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                            */
/* The copyright holder's institutional names and contributor's names may not */
/* be used to endorse or promote products derived from this software without  */
/* specific prior written permission of the institution or contributor.       */
/******************************************************************************/
  
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "XrdNet/XrdNetCache.hh"
#include "XrdNet/XrdNetSockAddr.hh"
#include "XrdSys/XrdSysPlatform.hh"
  
//------------------------------------------------------------------------------
//! The XrdNetAddrInfo class is meant to provide read/only access to members
//! that only class XrdNetAddr can manipulate. This allows the generic use of
//! the XrdNetAddr class but provides a means of exporting that information to
//! other objects without allowing those objects to modify the information.
//------------------------------------------------------------------------------

struct addrinfo;

class XrdNetAddrInfo
{
public:

//------------------------------------------------------------------------------
//! Provide our address family.
//!
//! @return Success: Returns AF_INET, AF_INET6, or AF_UNIX.
//!         Failure: Returns 0, address is not valid.
//------------------------------------------------------------------------------

inline int  Family() {return static_cast<int>(IP.addr.sa_family);}

//------------------------------------------------------------------------------
//! Format our address into a supplied buffer with one of the following layouts
//! (the ':<port>' or ':/path' can be omitted if desired, see fmtOpts param):
//! IP.xx:   host_name:<port>
//! IP.v4:   a.b.c.d:<port>
//! IP.v6:   [a:b:c:d:e:f:g:h]:<port> | [::a.b.c.d]:<port>
//! IP.Unix: localhost:/<path>
//!
//! @param  bAddr    address of buffer for result
//! @param  bLen     length  of buffer
//! @param  fmtType  specifies the type of format desired via fmtUse enum.
//! @param  fmtOpts  additional formatting options (can be or'd):
//!                  noPort   - do not append the port number to the address.
//!                  old6Map4 - use deprecated IPV6 mapped format '[::x.x.x.x]'
//!
//! @return Success: The number of characters (less null) in Buff.
//! @return Failure: 0 (buffer is too small or not a valid address). However,
//!                  if bLen > 0 the buffer will contain a null terminated
//!                  string of up to 8 question marks.
//------------------------------------------------------------------------------

enum fmtUse {fmtAuto=0, //!< Hostname if already resolved o/w use fmtAddr
             fmtName,   //!< Hostname if it is resolvable o/w use fmtAddr
             fmtAddr,   //!< Address using suitable ipv4 or ipv6 format
             fmtAdv6,   //!< Address only in ipv6 format
             fmtDflt};  //!< Use default format (see 2nd form of Format())

static const int noPort   = 0x0000001; //!< Do not add port number
static const int old6Map4 = 0x0000002; //!< Use deprecated IPV6 mapped format

int         Format(char *bAddr, int bLen, fmtUse fmtType=fmtDflt, int fmtOpts=0);

//------------------------------------------------------------------------------
//! Set the default format for all future calls of format. The initial default
//! is set to fmtName (i.e. if name is resolvable use it prefrentially).
//!
//! @param  fmtType  specifies the default of format desired via fmtUse enum.
//!
//! @return Always 1.
//------------------------------------------------------------------------------

int         Format(fmtUse fmtType)
                  {useFmt = (fmtType == fmtDflt ? fmtName : fmtType);
                   return useFmt;
                  }

//------------------------------------------------------------------------------
//! Indicate whether or not our address is the loopback address. Use this
//! method to gaurd against UDP packet spoofing.
//!
//! @return Success: Returns true if this is the loopback address.
//!         Failure: Returns false.
//------------------------------------------------------------------------------

bool        isLoopback();

//------------------------------------------------------------------------------
//! Indicate whether or not our address is registered in the DNS.
//!
//! @return Success: Returns true if this is registered.
//!         Failure: Returns false.
//------------------------------------------------------------------------------

bool        isRegistered();

//------------------------------------------------------------------------------
//! Convert our IP address to the corresponding [host] name.
//!
//! @param  eName    value to return when the name cannot be determined.
//! @param  eText    when not null, the reason for a failure is returned.
//!
//! @return Success: Pointer to the name or ip address with eText, if supplied,
//!                  set to zero. The memory is owned by the object and is
//!                  deleted when the object is deleted or Set() is called.
//!         Failure: eName param and if eText is not zero, returns a pointer
//!                  to a message describing the reason for the failure. The
//!                  message is in persistent storage and cannot be modified.
//------------------------------------------------------------------------------

const char *Name(const char *eName=0, const char **eText=0);

//------------------------------------------------------------------------------
//! Convert our IP address to the corresponding [host] name and return a copy
//! of the name. This method provides compatability to the XrdSysDNS method
//! getHostName() which always returned something.
//!
//! @param  eText    when not null, the reason for a failure is returned.
//!
//! @return Success: Pointer to the name or ip address with eText, if supplied,
//!                  set to zero. The name is an strdup'd string and must be
//!                  freed (using free()) by caller.
//!         Failure: strdup("0.0.0.0") and if eText is not zero, returns a
//!                  pointer to a message describing the reason for failure. The
//!                  message is in persistent storage and cannot be modified.
//------------------------------------------------------------------------------

char       *NameDup(const char **eText=0) {return strdup(Name("0.0.0.0",eText));}

//------------------------------------------------------------------------------
//! Provide a pointer to our socket address suitable for use in calls to methods
//! that require our internal format of sock addr. A value is only returned for
//! IPV6/4 addresses and is nill otherwise. The pointer refers to memory
//! allocated by this object and becomes invalid should the object be deleted.
//! Use SockSize() to get its logical length.
//------------------------------------------------------------------------------

inline const
XrdNetSockAddr  *NetAddr() {return (sockAddr == (void *)&IP ? &IP : 0);}

//------------------------------------------------------------------------------
//! Return the port number for our address.
//!
//! @return Success: The port number, which may be 0 if not set.
//!         Failure: -1 address is not an internet address or port is invalid.
//------------------------------------------------------------------------------

int         Port();

//------------------------------------------------------------------------------
//! Provide our protocol family.
//!
//! @return Success: Returns PF_INET, PF_INET6, or PF_UNIX.
//!         Failure: Returns 0, address is not valid.
//------------------------------------------------------------------------------

inline int  Protocol() {return static_cast<int>(protType);}

//------------------------------------------------------------------------------
//! Check if the IP address in this object is the same as the one passed.
//!
//! @param ipAddr    points to the network address object to compare.
//! @param plusPort  when true, port values must also match. In any case, both
//!                  addresses must be of the same address family.
//!
//! @return Success: True  (addresses are     the same).
//!         Failure: False (addresses are not the same).
//-----------------------------------------------------------------------------sav-

int         Same(const XrdNetAddrInfo *ipAddr, bool plusPort=false);

//------------------------------------------------------------------------------
//! Provide a pointer to our socket address suitable for use in calls to
//! functions that require one (e.g. bind() etc). The pointer refers to memory
//! allocated by this object and becomes invalid should the object be deleted
//! or when Set() is called. Use SockSize() to get its length.
//------------------------------------------------------------------------------

inline const
sockaddr   *SockAddr() {return sockAddr;}

//------------------------------------------------------------------------------
//! Provide the length of our socket adress. Useful for system calls needing it.
//!
//! @return Success: Returns the length of the address returned by SockAddr().
//!         Failure: Returns 0, address is not valid.
//------------------------------------------------------------------------------

inline
SOCKLEN_t   SockSize() {return addrSize;}

//------------------------------------------------------------------------------
//! Get the associated file descriptor.
//!
//! @return The associated file descriptor. If negative, no association exists.
//------------------------------------------------------------------------------

inline int  SockFD() {return sockNum;}

//------------------------------------------------------------------------------
//! Assignment operator
//------------------------------------------------------------------------------

XrdNetAddrInfo &operator=(XrdNetAddrInfo const &rhs)
               {if (&rhs != this)
                   {memcpy(&IP, &rhs.IP, sizeof(IP));
                    addrSize = rhs.addrSize; sockNum = rhs.sockNum;
                    if (hostName) free(hostName);
                    hostName = (rhs.hostName ? strdup(rhs.hostName):0);
                    if (rhs.sockAddr != &rhs.IP.addr)
                       {if (!unixPipe) unixPipe = new sockaddr_un;
                        memcpy(unixPipe, rhs.unixPipe, sizeof(sockaddr_un));
                       } else sockAddr = &IP.addr;
                   }
                return *this;
               }

//------------------------------------------------------------------------------
//! Copy constructor
//------------------------------------------------------------------------------

               XrdNetAddrInfo(XrdNetAddrInfo const &oP) {*this = oP;}

//------------------------------------------------------------------------------
//! Constructor
//------------------------------------------------------------------------------

            XrdNetAddrInfo() : hostName(0), addrSize(0), protType(0), sockNum(-1)
                           {IP.addr.sa_family = 0;
                            sockAddr = &IP.addr;
                           }

            XrdNetAddrInfo(const XrdNetAddrInfo *addr) {*this = *addr;}

//------------------------------------------------------------------------------
//! Destructor
//------------------------------------------------------------------------------

           ~XrdNetAddrInfo() {if (hostName) free(hostName);
                          if (sockAddr != &IP.addr) delete unixPipe;
                         }

protected:
       char               *LowCase(char *str);
       int                 QFill(char *bAddr, int bLen);
       int                 Resolve();

static XrdNetCache         dnsCache;
static fmtUse              useFmt;

// For optimization this union should be the first member of this class as we
// compare "unixPipe" with "&IP" and want it optimized to "unixPipe == this".
//
XrdNetSockAddr             IP;
union {struct sockaddr    *sockAddr;
       struct sockaddr_un *unixPipe;
      };
char                      *hostName;
SOCKLEN_t                  addrSize;
short                      protType;
short                      sockNum;
};
#endif

/*
MIT License

Copyright (c) 2019 Bharath Paulraj <bharathpaul@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/




#include <arpa/inet.h>
#include <stdlib.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_byteorder.h>
#include "main.h"
#include "netlink.h"
#include "routeds.h"
#include "xml/ezxml.h"
#include "logutil.h"

static int32_t doNlRecv (data_t *pData);
static int32_t nlCreate (void);

int32_t nlCreate (void)
{
#if 0
  /* destip: 10.254.0.0/24, outport = 1*/
  int32_t i32Ret = lpmAddv4Route (0x0AFE0000, 24, 1);
  if (i32Ret != 0)
  {
    printf ("route addition failed for 10.0.0.150/24 outport 1\n");
  }
  else
  {
    printf ("route addition success for 10.0.0.150/24 outport 1\n");
  }
#else

  ezxml_t cfg = ezxml_parse_file("route.xml");
  ezxml_t node =  ezxml_child(cfg, "route");
  uint32_t u32DestIp = 0;
  uint8_t u8Mask = 0;
  uint32_t u32NHop = 0;
  char *pBuff= NULL;
  int32_t i32Ret = 0;

  while (node)
  {
     pBuff = ezxml_child (node, "dest")->txt;
     i32Ret = inet_pton (AF_INET, pBuff, &u32DestIp);
     if (i32Ret != 1)
     {
       LOG_ERR ("Unable to parse IPv4 address %s", pBuff);
       node = node->next;
       continue;
     }
     pBuff = ezxml_child (node, "mask")->txt;
     u8Mask = atoi (pBuff);
     pBuff = ezxml_child (node, "nhop")->txt;
     u32NHop = atoi (pBuff);

     /* As NHop denotes port here, don't allow input greater than allowed port
      * numbers.
      */
     if (u32NHop >= vRouterg.u8PortCnt)
     {
       LOG_ERR ("Invalid next hop (%d) detected", u32NHop);
       node = node->next;
       continue;
     }

     u32DestIp = rte_cpu_to_be_32 (u32DestIp);
     i32Ret = lpmAddv4Route (u32DestIp, u8Mask, u32NHop);
     if (i32Ret == 0)
     {
       LOG_DEBUG ("route add %08x/%d via %d succeeded", u32DestIp, u8Mask, u32NHop);
     }
     else
     {
       LOG_WARN ("route add %08x/%d via %d failed", u32DestIp, u8Mask, u32NHop);
     }
     node = node->next;
  }
#endif
  return SUCCESS;
}

int32_t doNlRecv (data_t *pData)
{
  /* Until and unless KNI is implemented, netlink receive has 
   * no use
   */
  UNUSED_PARAM(pData);
  return SUCCESS;
}

static modules_t netLinkModule = {
  .name = "netLink",
  .pProcess = doNlRecv,
  .pCreate = nlCreate,
};

void registerNetLink (void)
{
  registerModules (&netLinkModule);
  return;
}

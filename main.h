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


#define RX_DESC 1024
#define TX_DESC 1024
#define MAX_PKT_BURST 32
#define MEMPOOL_CACHE_SIZE 256
#define MAX_MODULES 8
/* As of now keeping it as *2, but when the number of supported 
 * ports are increased, increase it accordingly
 */
#define MAX_MBUFS_TO_HOLD MAX_PKT_BURST * 2
#define SUCCESS 0
#define FAILURE -1
#define MAX_IPV4_ROUTES 128
#define UNUSED_PARAM(x) (void)(x)
#define RTE_LOGTYPE_VROUTER RTE_LOGTYPE_USER1

typedef struct _lcoreConf_
{
  uint16_t u16RxQ;
  uint16_t u16TxQ;
  uint8_t u8CoreId;
}lcoreConf_t;

typedef struct _portConf_
{
  uint8_t u8RxQCnt;
  uint8_t u8TxQCnt;
  uint16_t u16PortId;
  struct ether_addr macAddr;
}portConf_t;

typedef struct _data_
{
  struct rte_mbuf *pMbuf[MAX_MBUFS_TO_HOLD];
  uint16_t u16MbufCnt;
}data_t;

typedef int32_t processFunc (data_t *pData);
typedef int32_t createFunc (void);
typedef struct _modules_
{
  char name[32];
  processFunc *pProcess;
  createFunc *pCreate;
}modules_t;

typedef struct _metadata_
{
  uint32_t u32NHopIp;
  uint8_t u8OutPort;
  /* Other Params comes here */
}metaData_t;

typedef struct _vrouteGbls_
{
  char cTimeStamp[128];
  lcoreConf_t lcoreConf[RTE_MAX_LCORE];
  portConf_t portConf[RTE_MAX_ETHPORTS];
  modules_t *pModule[MAX_MODULES];
  uint8_t u8PortCnt;
  uint8_t u8CoreCnt;
  uint8_t u8ModuleCnt;
  struct rte_mempool *pMempool;
  struct rte_lpm *pLpmv4;
  uint32_t u32LogLevel;
}vRouterG_t;

vRouterG_t vRouterg;

extern void registerModules (modules_t *pModules);


/*
 * TcpSyncAdapter.h
 *
 *  Created on: 2020/01/20 10:05:04
 *      Author: juntao
 */

#ifndef INCLUDE_STREAM_TCPSYNCADAPTER_H_
#define INCLUDE_STREAM_TCPSYNCADAPTER_H_
#include <string>
using namespace std;
namespace iono
{
	class TcpSyncAdapter
	{
	public:
		TcpSyncAdapter();
		~TcpSyncAdapter();
		bool m_connect(const char *);
		int m_readSync(char *buff, int len);
		void m_close();

	protected:
		int m_setsock(int);
		int m_openTcp(string ip, int);
		string ip;
		int port;
		int sockid;
	};
}
#endif /* INCLUDE_STREAM_TCPSYNCADAPTER_H_ */

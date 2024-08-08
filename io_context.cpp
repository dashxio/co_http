#include "io_context.h"
#include "http_server.h"

callback<> io_context::_getQueuedCompletionPackage()
{
	DWORD io_bytes_num = 0;
	CompletionKey* com_key_ptr = NULL;
	LPOVERLAPPED lp = NULL;
	convert_error<FALSE, ForGetLastError>(GetQueuedCompletionStatus(m_cphd, &io_bytes_num, (PULONG_PTR)&com_key_ptr, &lp, INFINITE)).expect("Get queued completion status");
	auto h = std::unique_ptr<HttpServer::HttpServerSkin>((HttpServer::HttpServerSkin*)lp);
	return [h = std::move(h), io_bytes_num] {
		h->update(io_bytes_num);
		};
}

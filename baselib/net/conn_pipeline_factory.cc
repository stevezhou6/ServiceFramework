
#include "net/conn_pipeline_factory.h"

#include <wangle/channel/EventBaseHandler.h>

#include "net/thread_local_conn_manager.h"

const uint64_t kDefaultMinAvailable = 8096;
const uint64_t kDefaultAllocationSize = 16192;

///////////////////////////////////////////////////////////////////////////////////////////
ConnPipeline::Ptr ConnPipelineFactory::newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) {
    VLOG(4) << "ConnPipelineFactory::newPipeline!";
    auto pipeline = ConnPipeline::create();
    pipeline->addBack(wangle::AsyncSocketHandler(sock));
    pipeline->addBack(wangle::EventBaseHandler()); // ensure we can write from any thread
//    pipeline->addBack(CImPduRawDataDecoder());
//    pipeline->addBack(StatisticsConnHandler());
//    pipeline->addBack(IMConnHandler(service_));

    pipeline->finalize();

    return pipeline;
}

///////////////////////////////////////////////////////////////////////////////////////////
ConnPipeline::Ptr ClientConnPipelineFactory::newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) {
    VLOG(4) << "ClientConnPipelineFactory::newPipeline!";
    auto pipeline = ConnPipeline::create();
    pipeline->setReadBufferSettings(1024*32, 1024*64);

    // Initialize TransportInfo and set it on the pipeline
    auto transportInfo = std::make_shared<wangle::TransportInfo>();
    folly::SocketAddress localAddr, peerAddr;
    sock->getLocalAddress(&localAddr);
    sock->getPeerAddress(&peerAddr);
    transportInfo->localAddr = std::make_shared<folly::SocketAddress>(localAddr);
    transportInfo->remoteAddr = std::make_shared<folly::SocketAddress>(peerAddr);
    pipeline->setTransportInfo(transportInfo);

    pipeline->addBack(wangle::AsyncSocketHandler(sock));
    pipeline->addBack(wangle::EventBaseHandler()); // ensure we can write from any thread
//    pipeline->addBack(CImPduRawDataDecoder());
//    pipeline->addBack(StatisticsConnHandler());
//    pipeline->addBack(IMConnHandler(service_));
    pipeline->finalize();

    return pipeline;
}

///////////////////////////////////////////////////////////////////////////////////////////
ConnPipeline::Ptr ServerConnPipelineFactory::newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) {
    VLOG(4)<< "ServerConnPipelineFactory::newPipeline!";
    auto pipeline = IMConnPipeline::create();
    pipeline->setReadBufferSettings(1024*32, 1024*64);
    pipeline->addBack(wangle::AsyncSocketHandler(sock));
//    pipeline->addBack(CImPduRawDataDecoder());
//    pipeline->addBack(StatisticsConnHandler());

    //pipeline->addBack(IMConnHandler(service_));
    pipeline->finalize();

    return pipeline;
}

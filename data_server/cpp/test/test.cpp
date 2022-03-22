#include "test.h"
#include "../Log/log.h"
#include "../grpc_comm/server.h"
#include "../db_engine/db_engine.h"

void test_grpc_server()
{
    GrpcServer server("0.0.0.0:5008");

    server.start();    
}

class TestRpc
{
public:
    TestRpc(string address):address_{address}, responder_{&context_}
    {
        init_server_local();

        start_local();
    }

    TestRpc(MarketService::AsyncService* service, ServerCompletionQueue* cq):
            p_service_{service}, p_cq_{cq}, responder_{&context_}
    {
        start_rpc();
    }    

    void init_server_local()
    {
        LOG_INFO("[RPC] TestServer listen: " + address_);

        builder_.AddListeningPort(address_, grpc::InsecureServerCredentials());
        builder_.RegisterService(&service_);

        u_cq_ = builder_.AddCompletionQueue();
        server_ = builder_.BuildAndStart();
    }

    void start_local()
    {
        LOG_INFO("[RPC] Start RequestRequestTradeData");

        service_.RequestRequestTradeData(&context_, &request_info_, &responder_, u_cq_.get(), u_cq_.get(), this);        
    }

    void start_rpc()
    {
        LOG_INFO("[Server] Start RequestRequestTradeData");
        
        p_service_->RequestRequestTradeData(&context_, &request_info_, &responder_, p_cq_, p_cq_, this);
    }

public:
    string                                  address_;
        
    std::unique_ptr<grpc::ServerCompletionQueue>  u_cq_;
    MarketService::AsyncService                   service_;
    std::unique_ptr<grpc::Server>                 server_;
    grpc::ServerBuilder                           builder_;

    
    grpc::ServerCompletionQueue*             p_cq_;
    MarketService::AsyncService*             p_service_;

    ServerContext                            context_;
    ReqTradeInfo                             request_info_;
    Proto3::MarketData::TradeData                                reply_info_;
    ServerAsyncResponseWriter<Proto3::MarketData::TradeData>     responder_;    

    std::thread                                   cq_thread_;
};


class TestServer
{
    public:
        TestServer(string address): address_{address} 
        {
            if (is_local_rpc_)
            {
                test_rpc_ = new TestRpc(address);
            }
            else
            {
                init_server(); 

                start();
            }
            
        }

        ~TestServer()
        {
            if (cq_thread_.joinable())
            {
                LOG_INFO("cq_thread join");
                cq_thread_.join();
            }
        }

        void init_server()
        {
            LOG_INFO("[Server] TestServer listen: " + address_);

            grpc::ServerBuilder                           builder;
            builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
            builder.RegisterService(&service_);

            u_cq_ = builder.AddCompletionQueue();
            u_server_ = builder.BuildAndStart();

            test_rpc_ = new TestRpc(&service_, u_cq_.get());
        }

        void start()
        {
            // cq_loop_main();

            start_cq_thread();
        }

        void start_cq_thread()
        {
            cq_thread_ = std::thread(&TestServer::cq_loop_main, this);
        }

        void cq_loop_main()
        {
            void* tag;  // uniquely identifies a request.
            bool ok;

            while (true) {

                // Block waiting to read the next event from the completion queue. 
                // The event is uniquely identified by its tag, 
                // which in this case is the memory address of a CallData instance.
                // The return value of Next should always be checked. This return value
                // tells us whether there is any kind of event or cq_ is shutting down.
                GPR_ASSERT(u_cq_->Next(&tag, &ok));

                cout << "Call Cq " << tag  << " ok: " << ok << endl;

                GPR_ASSERT(ok);

                cout << "Next Event From Completion queue " << endl;

                // static_cast<TestRpc*>(tag)->Proceed();
            }
        }

    private:
    bool                                          is_local_rpc_{false};
    string                                        address_;

    std::unique_ptr<grpc::ServerCompletionQueue>  u_cq_;
    MarketService::AsyncService                   service_;    
    std::unique_ptr<grpc::Server>                 u_server_;

    
    
    std::thread                                   cq_thread_;
    TestRpc*                                      test_rpc_;
};

class ServerImpl final {
 public:
  ~ServerImpl() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("0.0.0.0:50051");

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();

    std::cout << "ServerImpl Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs();
  }

 private:
  // Class encompasing the state and logic needed to serve a request.
    class CallData 
    {
      public:
        // Take in the "service" instance (in this case representing an asynchronous
        // server) and the completion queue "cq" used for asynchronous communication
        // with the gRPC runtime.
        CallData(MarketService::AsyncService* service, ServerCompletionQueue* cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) 
        {
          // Invoke the serving logic right away.
          Proceed();
        }

        void Proceed() 
        {
          cout << "\n***** Start Proceed! *****" << endl;
          if (status_ == CREATE) 
          {
            // Make this instance progress to the PROCESS state.
            status_ = PROCESS;

            cout << "Status == CREATE, this address: " << this << endl;

            // As part of the initial CREATE state, we *request* that the system
            // start processing SayHello requests. In this request, "this" acts are
            // the tag uniquely identifying the request (so that different CallData
            // instances can serve different requests concurrently), in this case
            // the memory address of this CallData instance.
            service_->RequestRequestTradeData(&ctx_, &request_, &responder_, cq_, cq_, this); // Call Cq Next?

            cout << "service_->RequestSayHello this: " << this << "\n" << endl;

          } 
          else if (status_ == PROCESS) 
          {
            cout << "Status == PROCESS, this address: " << this << endl;

            // Spawn a new CallData instance to serve new clients while we process
            // the one for this CallData. The instance will deallocate itself as
            // part of its FINISH state.

            new CallData(service_, cq_);

            // The actual processing.
            std::string prefix("Hello ");
            // reply_.set_message(prefix + request_.name());

            // And we are done! Let the gRPC runtime know we've finished, using the
            // memory address of this instance as the uniquely identifying tag for
            // the event.
            status_ = FINISH;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            responder_.Finish(reply_, Status::OK, this);  // Call Cq Next ?
            cout << "responder_.Finish  this address: " << this << "\n" << endl;
          } 
          else 
          {
            cout << "Status == FINISH, this address: " << this << "\n" << endl;
            GPR_ASSERT(status_ == FINISH);

            status_ == CREATE;

            // Once in the FINISH state, deallocate ourselves (CallData).
            delete this;
          }
        }

      private:
        // The means of communication with the gRPC runtime for an asynchronous
        // server.
        MarketService::AsyncService* service_;
        // The producer-consumer queue where for asynchronous server notifications.
        ServerCompletionQueue* cq_;
        // Context for the rpc, allowing to tweak aspects of it such as the use
        // of compression, authentication, as well as to send metadata back to the
        // client.
        ServerContext ctx_;

        // What we get from the client.
        ReqTradeInfo request_;
        // What we send back to the client.
        Proto3::MarketData::TradeData reply_;

        // The means to get back to the client.
        ServerAsyncResponseWriter<Proto3::MarketData::TradeData> responder_;

        // Let's implement a tiny state machine with the following states.
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_;  // The current serving state.
    };

    // This can be run in multiple threads if needed.
    void HandleRpcs() {
      // Spawn a new CallData instance to serve new clients.
      new CallData(&service_, cq_.get());
      void* tag;  // uniquely identifies a request.
      bool ok;
      while (true) {

        // Block waiting to read the next event from the completion queue. 
        // The event is uniquely identified by its tag, 
        // which in this case is the memory address of a CallData instance.
        // The return value of Next should always be checked. This return value
        // tells us whether there is any kind of event or cq_ is shutting down.
        GPR_ASSERT(cq_->Next(&tag, &ok));

        cout << "Call Cq " << tag  << " ok: " << ok << endl;

        GPR_ASSERT(ok);

        cout << "Next Event From Completion queue " << endl;

        static_cast<CallData*>(tag)->Proceed();
      }
    }

  std::unique_ptr<ServerCompletionQueue>        cq_;
  MarketService::AsyncService                   service_;
  std::unique_ptr<Server>                       server_;
};

void test_non_stream_server()
{
    TestServer test_server ("0.0.0.0:5008");
}

void test_official_demo()
{
    ServerImpl server_obj;
    server_obj.Run();
}

void test_db_engine()
{
  TestEngine test_engine;
  test_engine.start();
}

void TestMain()
{
    LOG_INFO("TestMain");
    
    test_grpc_server();

    // test_db_engine();

    // test_non_stream_server();

    // test_official_demo();q
}
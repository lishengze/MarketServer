#include "client.h"

void test_put_order_stream()
{
    SyncClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    client.PutOrderStream();
}

int main()
{
    cout << "Test Risk Grpc" << endl;

    return 1;
}
#include "client.h"

void test_put_order_stream()
{
    SyncClient client(grpc::CreateChannel("localhost:9111", grpc::InsecureChannelCredentials()));

    client.PutOrderStream();
}

int main()
{
    cout << "Test Risk Grpc" << endl;

    test_put_order_stream();

    return 1;
}
#include <iostream>
#include <thread>

int main()
{
    while(true){
        cout << "hello" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
#include <iostream>
using namespace std;

int main() {
	char value[1024];
	sprintf(value, "%f", 0.165);
	cout << value << endl;
	return 0;
};

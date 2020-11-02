#include <iostream>
#include <string>
#include <vector>
#include <any>
using namespace std;

struct Test
{
	int a;
	double b;
};

int main() {
	vector<any> container;
	container.resize(3);
	container[0] = 42;
	container[1] = std::string("hi");
	container[2] = new Test();

	for( const auto& v : container ) {
		if( v.type() == typeid(int) ) {
			std::cout << "int:" << std::any_cast<const int&>(v) << std::endl;
		} else if( v.type() == typeid(string) ) {
			std::cout << "string:" << std::any_cast<const string&>(v) << std::endl;
		} else if( v.type() == typeid(Test*) ) {
			std::cout << "Test:" << std::any_cast<const Test*>(v)->a << std::endl;
		} else {
			std::cout << "unknown" << std::endl;
		}
	}
	return 0;
};

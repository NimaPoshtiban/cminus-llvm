#include "cminus.h"
#include <string>
#include <iostream>
int main() {
	const std::string program = R"(
	version;
	i32 hi(){
    }
)";
	Cminus cm{ program };
	cm.exec();
}
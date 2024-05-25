#include "cminus.h"
#include <string>
#include <iostream>
int main() {
	const std::string program = R"(
	printf("Hello Nima")
)";
	Cminus cm{ program };
	cm.exec();
}
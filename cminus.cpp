#include "cminus.h"
#include <string>
#include <iostream>
int main() {
	const std::string program = R"(
	version;
	let b = 11 * 11
	b
	let nima = -112;
	nima
	123
	12.1
	i1 hi(i32 age,i64 salary){
	
    }

)";
	Cminus cm{ program };
	cm.exec();
}
#include "cminus.h"
#include <string>
#include <iostream>
int main() {
	const std::string program = R"(
	version;
	let b = 11 * 11
	let salary = 5000.128 + 0.12
	let nima = -112;
	{
		let nima = 12;
		nima
	}
	if(b==121){
	  let result = true;
	}else{
      let result = false;
	}
	
	i1 hi(i32 age,i64 salary){
	
    }

)";
	Cminus cm{ program };
	cm.exec();
}
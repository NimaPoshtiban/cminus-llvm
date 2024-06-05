#include "cminus.h"
#include <string>
#include <iostream>
int main() {
	const std::string program = R"(
	version;
	let b = 11 * 11;
	let salary = 5000.128 + 0.12
	let nima = -112;
	{
		mut salary = salary + 20.2;
		let nima = 12;
	}
	let numbers = [1,12,23,43];
	if(b==121){
	  let result = true;
	  
	}else{
      let result = false;
	}

	while(b > 3){
	  mut b = b-1
	}

	i1 hi(i32 age,i64 salary){
		mut age = age + 2;
		 false
    }

	mut b = hi(113,21);
	}
)";
	Cminus cm{ program };
	cm.exec();
}
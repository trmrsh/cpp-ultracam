#include <iostream>

using namespace std;

int main(){

  int i1 = 4027;
  int i2 = 0xFBB;

  if(i1 == i2)
    std::cout << "numbers are equal" << i1 << " " << i2 << std::endl;
  else
    std::cout << "numbers are not equal" << i1 << " " << i2 << std::endl;

}

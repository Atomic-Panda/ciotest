#include<iostream>
using namespace std;
int main(){
    for(int i = 0; i < 256; i++){
        int t = -128 + i;
        cout << i <<" " << (char)t << " "  << (int)(char)t<< endl;
    }
    cout <<endl;
    return 0;
}
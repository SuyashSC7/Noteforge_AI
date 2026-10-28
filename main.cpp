#include <iostream>
#include <cpr/cpr.h>
using namespace std;

int main() {
    auto response = cpr::Get(
        cpr::Url{"https://generativelanguage.googleapis.com"},
        cpr::Timeout{10000}
    );

    cout << "Status: " << response.status_code << '\n';
    cout << "Error: " << response.error.message << '\n';
}
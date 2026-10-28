#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cstdlib>

using namespace std;
using json= nlohmann::json;


string askai(string prompt){
    
    const char* key= getenv("groq_api_key");

    if (key==nullptr){
        return "no api key\n";
    }

    string url = "https://api.groq.com/openai/v1/chat/completions";

    json requestBody = {

        {"model", "openai/gpt-oss-20b"},

        {"messages", json::array({

            {

                {"role", "user"},

                {"content", prompt}

            }

        })}

    };

    cpr:: Response res= cpr::Post(
        cpr::Url{url},

        cpr::Header{
            {"Authorization", string("Bearer ") + key},
            {"Content-Type", "application/json"}
        },

        cpr::Body{requestBody.dump()},

        cpr::Timeout{30000}

    );

    if (res.error){
        cerr<<"network error"<< res.error.message<<'\n';
        return "";
    }

    if (res.status_code!=200){
        cerr<<"http error"<< res.status_code<<'\n'<< res.text<<'\n';
        return "";
    }

    try{
        json result= json::parse(res.text);
        return result["choices"][0]["message"]["content"];

    }

    catch (const json::exception& e){
        cerr<<"could not parse response"<<e.what()<<'\n';
        return "";
    }
    return 0;
}

int main(){
    cout<<" WELCOME TO NOTEFORGE";

    string prompt;

    while (true){
        cout<<":";
        getline(cin,prompt);
        if (prompt=="c"){
            return 0;
        }
        else{
            cout<<askai(prompt);
            cout<<'\n';
        }
    }

    return 0;
}



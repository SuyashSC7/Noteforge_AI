#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cstdlib>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cstdio>


using namespace std;
using json= nlohmann::json;
namespace fs= std::filesystem;
const string PYTHON = "/Users/suyashsudhakarchavan/Desktop/projects/noteforge_ai/.venv/bin/python3";
const string SCRIPT = "/Users/suyashsudhakarchavan/Desktop/projects/noteforge_ai/extract.py";

//for tracking history
json history=json::array();



vector<fs::path> findtextfile(const string& folder){
    
    vector<fs::path> files;     //for storing the path

    try{
        for (const auto& entry : fs::directory_iterator(folder)){   //iterator iterates through all the files

            string ext = entry.path().extension().string();       //for getting the extension 
            transform( ext.begin(),ext.end(),ext.begin(), ::tolower);

            if (entry.is_regular_file() && (ext==".txt" || ext==".md" || ext == ".pdf" || ext == ".docx" || ext == ".pptx")){
                files.push_back(entry.path());      //adding path to files vector
            }
            
        }
    }
    catch(const fs::filesystem_error& e){
        cerr<<"could not read folder: "<<e.what()<<"\n";        //error catching
    }

    return files;
}

//api call
string askai(json history){
    
    const char* key= getenv("groq_api");        //key in the envirnment variable of system

    if (key==nullptr){
        return "no api key\n";
    }

    string url = "https://api.groq.com/openai/v1/chat/completions";

    //type of request needed to send for groq
    json requestBody = {

        {"model", "openai/gpt-oss-20b"},
        {"messages",history},
        {"stream",true}

    };

    string pending;
    string full;
    string rawbody;

    auto handleline= [&](const string& line){

        if (line.rfind("data: ",0)!=0) return;  //no data entered

        string payload=line.substr(6);      //first is [done] so skip that and then put that as payload

        if (payload == "[DONE]") return;

        try {
            json chunk =json::parse(payload);

            const json& delta=chunk["choices"][0]["delta"];

            if (delta.contains("content") && delta["content"].is_string()) {
                string piece = delta["content"].get<string>();
                cout << piece << flush;
                full +=piece;
            }
        }

        catch ( const json::exception& ){}
    };


    //handling the response
    cpr:: Response res= cpr::Post(
        cpr::Url{url},

        cpr::Header{
            {"Authorization", string("Bearer ") + key},
            {"Content-Type", "application/json"}
        },

        cpr::Body{requestBody.dump()},

        cpr::Timeout{30000},

        cpr::WriteCallback{

            [&](const std::string_view& data, intptr_t) -> bool {
                if (rawbody.size()<4000) rawbody += data;

                pending+=data;
                
                size_t pos;

                while ((pos=pending.find('\n'))!= string::npos){

                    string line = pending.substr(0,pos);
                    pending.erase(0,pos+1);

                    if (!line.empty() && line.back()=='\r') line.pop_back();

                    handleline(line);

                }

                return true;

            }

        }
    );

    if (res.error){
        cerr<<"network error"<<res.error.message<<'\n';
        return "";
    }

    if ( res.status_code != 200){
        cerr<<"http error"<< res.status_code<<'\n'<<rawbody<<'\n';
        return "";
    }
    return full;
}

string run_comand(const string& cmd , int& status){
    FILE* pipe=popen(cmd.c_str(),"r");

    if (!pipe){
        status=-1;
        return "";
    }

    char buf[4096];
    string output;

    while(fgets(buf,sizeof(buf),pipe)){
        output += buf;
    }


    status=pclose(pipe);

    return output;
}

string shellquote(const string& s){
    string out="'";
    for (char c: s){
        if ( c== '\'' ){
            out+="'\\''";
        }
        else{
            out+=c;
        }
    }
    out += "'";
    return out;
}

string extract_text(const fs::path& p) {
    string ext = p.extension().string();
    transform( ext.begin(),ext.end(),ext.begin(), ::tolower);

    if (ext == ".txt" || ext == ".md") {
        ifstream file(p);
        if (!file) return "";

        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    else if ( ext == ".pdf"){
        string cmd = "pdftotext -layout " + shellquote(p.string()) + " -";
        int status=0;
        string text = run_comand(cmd , status);
        
        if ( status!=0 ){
            cerr << p.filename().string() << "- pdftotextfailed \n";
            return "";
        }

        return text;
    }

    else if ( ext == ".docx" || ext == ".pptx" ){
        string cmd = "\"" + PYTHON + "\" \"" + SCRIPT + "\" \"" + p.string() + "\"";
        int status = 0;
        string text = run_comand( cmd , status );
        
        if (status != 0){
            cerr << p.filename().string() << ": extract.py failed \n";
            return "" ;
        }

        return text;

    }
   

    // .pdf, .docx and .pptx come later
    cerr << p.filename().string() << ": unsupported file\n";
    return "";
}

void readfile(const fs::path& p) {
    string text = extract_text(p);

    if (text.empty()) {
        cerr << "skipped (no text): " << p.filename().string() << '\n';
        return;
    }

    history.push_back({
        {"role", "system"},
        {"content", "file name: " + p.filename().string() + "\n" + text + "\n"}
    });
}



int main(int argc,char* argv[]){
    //to check whther file is specified
    if ( argc<2 ){
        cerr<<"folder for resources not there\n";
        return 1;
    }

    string folder= argv[1];
    
    //checking the name
    if (!fs::exists(folder) || !fs::is_directory(folder)){
        cerr<<"no such folder exist\n";
        return 1;
    }

    std::cout<<"WELCOME TO NOTEFORGE"<<'\n';

    string prompt;

    history.push_back({
        {"role","system"},
        {"content","you are a world class study notes maker, quizzer, question paper maker from all the documents and files whih are given in this message you will do all the things user wants you to do be precise carefull and accurate"}
    });


    //for file reading
    vector<fs::path> files=findtextfile(folder);

    for(auto &file : files){
        readfile(file);
    }
    json base = history;


    while (true){

        std::cout<<":";

        if(!getline(cin,prompt)) break;


        if (prompt=="c"){
            return 0;
        }


        else{
            if (prompt=="clear"){

                history=base;
                std::cout<<"HISTORY CLEARED"<<'\n';

                continue;
            }

            history.push_back({
                {"role","user"},
                {"content",prompt}
            });
            
            string response = askai(history);

            std::cout<< '\n';
            
            if (!response.empty()){
                history.push_back({

                    {"role","assistant"},
                    {"content", response}
                });
            }
        }
    }

    return 0;
}



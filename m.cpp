    #include <iostream>
    #include <cpr/cpr.h>
    #include <nlohmann/json.hpp>
    #include <string>
    #include <cstdlib>
    #include <sstream>
    #include <filesystem>
    #include <fstream>

    using namespace std;
    using json= nlohmann::json;
    namespace fs= std::filesystem;
    json history=json::array();
    

    void readfile(const fs::path& p){
        ifstream file(p);

        if (!file) return;

        stringstream buffer;
        buffer<< file.rdbuf();

        history.push_back({
            {"role","system"},
            {"content","file name: "+p.filename().string() + "\n" + buffer.str()+"\n"}
        });

    }

    vector<fs::path> findtextfile(const string& folder){
        vector<fs::path> files;
        try{
            for (const auto& entry : fs::directory_iterator(folder)){

                string ext=entry.path().extension().string();

                if (entry.is_regular_file() && (ext==".txt" || ext==".md")){
                    files.push_back(entry.path());
                }
                
            }
        }
        catch(const fs::filesystem_error& e){
            cerr<<"could not read folder: "<<e.what()<<"\n";
        }

        return files;
    }


    //api call
    string askai(json history){
        
        const char* key= getenv("groq_api");

        if (key==nullptr){
            return "no api key\n";
        }

        string url = "https://api.groq.com/openai/v1/chat/completions";

        json requestBody = {

            {"model", "openai/gpt-oss-20b"},
            {"messages",history},
            {"stream",true}

        };

        string pending;
        string full;
        string rawbody;

        auto handleline= [&](const string& line){
            if (line.rfind("data: ",0)!=0) return;
            string payload=line.substr(6);
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
            catch ( const json::exception&){}
        };


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



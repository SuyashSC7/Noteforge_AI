#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cstdlib>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <vector>

using namespace std;
using json= nlohmann::json;
namespace fs= std::filesystem;

const string PYTHON = "/Users/suyashsudhakarchavan/Desktop/projects/noteforge_ai/.venv/bin/python3";
const string SCRIPT = "/Users/suyashsudhakarchavan/Desktop/projects/noteforge_ai/extract.py";

const string notes_prompt =
"You are an academic notes generator. Using ONLY the files provided, create comprehensive, structured, and exam-oriented study notes in Markdown.\n\n"

"Follow these instructions:\n"
"1. Process each file separately and use its filename as the main heading.\n"
"2. Organize the content into logical topics and subtopics with appropriate headings.\n"
"3. Explain key concepts using concise, easy-to-understand bullet points.\n"
"4. Highlight all important definitions in **bold**.\n"
"5. Write mathematical formulas, equations, and important expressions on separate lines.\n"
"6. Include relevant examples, diagrams (using Markdown where possible), and step-by-step explanations if they are present in the source files.\n"
"7. Emphasize important points, classifications, differences, advantages, disadvantages, and applications that may be relevant for exams.\n"
"8. Avoid unnecessary repetition, filler content, and overly lengthy explanations.\n"
"9. Do not introduce external information or invent facts. If something is unclear or missing, explicitly mention it rather than guessing.\n"
"10. End each file's notes with a section titled 'Likely Exam Questions', containing relevant questions based strictly on that file's content.\n"
"11. If a file contains no readable or useful content, mention that clearly instead of generating notes.\n\n"

"The final output must be well-formatted Markdown, easy to revise, and suitable for university examination preparation.";

const string QUIZ_PROMPT =
    "Using only the files provided above, create exactly 10 multiple-choice questions. "
    "Return ONLY a JSON array, with no text before or after it and no markdown fences. "
    "Use exactly this shape:\n"
    R"json([{"question": "...", "options": ["...", "...", "...", "..."], "answer": "B", "explanation": "..."}])json"
    "\n"
    "The answer is a single capital letter A to D matching the position in options. "
    "Do not put letters like A) in front of the options. "
    "Do not invent facts that are not in the files.";

//for tracking history
json history=json::array();


bool savetofile(const fs::path& path , const string& text){
    std::error_code ec;

    if (!path.parent_path().empty()){
        fs::create_directories(path.parent_path(),ec);

        if (ec){
            cerr << "could not create folder "<<path.parent_path().string()<<" error : "<<ec.message()<<"\n";
            return false;
        }
    }

    ofstream file(path);

    if (!file){
        cerr << "could not open the file "<<path.string()<<" for writing.\n";
        return false;
    }

    file << text;
    return file.good();
}

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
string askai(const json& history, bool show = true){
    
    const char* key= getenv("groq_api");        //key in the envirnment variable of system

    if (key==nullptr){
        cerr<<"no api key\n";
        return "";
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
                if (show==true) cout << piece << flush;
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

        cpr::Timeout{120000},

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
        string cmd = shellquote(PYTHON) + " " + shellquote(SCRIPT) + " " + shellquote(p.string());
        int status = 0;
        string text = run_comand( cmd , status );
        
        if (status != 0){
            cerr << p.filename().string() << ": extract.py failed \n";
            return "" ;
        }

        return text;

    }
   

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

void generate(const string& instructions, const fs::path& outfile){
    json request=history;

    request.push_back({
        {"role","user"},
        {"content",instructions}
    });

    string response=askai(request);
    cout<<"\n";

    if (response.empty()){
        cerr << "could not generate notes \n";
        return;

    }
    if ( savetofile(outfile,response)){
        cout << "notes saved to " << outfile.string() << '\n';
    }
    return;
}



json parsequiz(const string& response){

    size_t start= response.find('[');
    size_t end= response.rfind("]");
    
    if ( start == string::npos || end == string::npos || end<start){
        cerr << "could not find any question in the output \n";
        return json::array();
    }

    try{
        json quiz = json::parse(response.substr(start,end-start+1));

        if (!quiz.is_array()){ cerr<<"not an array\n";return json::array();}

        return quiz;
    }
    catch (const json::exception& e){
        cerr<<"could not parse quiz"<<e.what()<<"\n";
        return json::array();
    }
}
json validate_quiz(const json& questions){
    json clean = json::array();

    for (size_t i = 0; i < questions.size(); i++){
        const json& q = questions[i];
        size_t n = i + 1;   // question number as a person counts it

        if (!q.is_object() || !q.contains("question") || !q.contains("options") || !q.contains("answer")){
            cerr << "question " << n << " skipped: missing fields\n";
            continue;
        }

        if (!q["question"].is_string() || !q["options"].is_array() || !q["answer"].is_string()){
            cerr << "question " << n << " skipped: a field has the wrong type\n";
            continue;
        }

        if (q["options"].size() != 4){
            cerr << "question " << n << " skipped: needs exactly 4 options\n";
            continue;
        }

        bool ok = true;
        for (const auto& opt : q["options"]){
            if (!opt.is_string()){ ok = false; break; }
        }
        if (!ok){
            cerr << "question " << n << " skipped: an option is not text\n";
            continue;
        }

        string ans = q["answer"].get<string>();
        if (ans != "A" && ans != "B" && ans != "C" && ans != "D"){
            cerr << "question " << n << " skipped: answer must be A, B, C or D\n";
            continue;
        }

        clean.push_back(q);
    }
    return clean;
}

// returns 1-4, or 0 if the user quits / input ends
int readchoice(){
    while (true){
        cout << "your answer (1-4, or q to quit): ";
        string line;
        if (!getline(cin, line)) return 0;

        if (line.size() == 1){
            if (line[0] >= '1' && line[0] <= '4') return line[0] - '0';
            if (line[0] == 'q' || line[0] == 'Q') return 0;
        }
        cout << "please type 1, 2, 3 or 4 (or q to quit)\n";
    }
}

void quiz(){
    json request = history;

    request.push_back({
        {"role", "user"},
        {"content", QUIZ_PROMPT}
    });

    cout << "generating quiz ...\n";

    string response = askai(request, false);
    if (response.empty()){
        cerr << "quiz could not be generated\n";
        return;
    }

    json questions = parsequiz(response);
    if (questions.empty()){
        cerr << "no usable questions generated\n";
        return;
    }

    json clean = validate_quiz(questions);
    if (clean.empty()){
        cerr << "no valid questions found\n";
        return;
    }
    cout << "kept " << clean.size() << " of " << questions.size() << " questions\n\n";

    vector<int> user_answers;

    for (size_t i = 0; i < clean.size(); i++){
        const json& q = clean[i];

        cout << "Q" << i + 1 << "/" << clean.size() << ": " << q["question"].get<string>() << '\n';
        for (int j = 0; j < 4; j++){
            cout << "  " << j + 1 << ") " << q["options"][j].get<string>() << '\n';
        }

        int choice = readchoice();
        if (choice == 0){
            cout << "\nquiz stopped\n";
            break;
        }
        user_answers.push_back(choice);
        cout << '\n';
    }

    size_t asked = user_answers.size();
    if (asked == 0) return;

    int score = 0;
    for (size_t i = 0; i < asked; i++){
        int correct = clean[i]["answer"].get<string>()[0] - 'A' + 1;
        if (correct == user_answers[i]) score++;
    }

    cout << "your score: " << score << " / " << asked << '\n';

    // review the mistakes
    for (size_t i = 0; i < asked; i++){
        const json& q = clean[i];
        int correct = q["answer"].get<string>()[0] - 'A' + 1;
        if (correct == user_answers[i]) continue;

        cout << "\nQ" << i + 1 << ": " << q["question"].get<string>() << '\n';
        cout << "  your answer:    " << q["options"][user_answers[i] - 1].get<string>() << '\n';
        cout << "  correct answer: " << q["options"][correct - 1].get<string>() << '\n';
        if (q.contains("explanation") && q["explanation"].is_string()){
            cout << "  why: " << q["explanation"].get<string>() << '\n';
        }
    }
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

            else if (prompt == "notes" ){
                generate(notes_prompt,"output/notes.md");
                continue;
            }

            else if ( prompt == "quiz" ){
                quiz();
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



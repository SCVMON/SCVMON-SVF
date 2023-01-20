//===- svf-ex.cpp -- A driver example of SVF-------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===-----------------------------------------------------------------------===//

/*
 // A driver program of SVF including usages of SVF APIs
 //
 // Author: Yulei Sui,
 */

#include "SVF-FE/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-FE/PAGBuilder.h"
#include "Util/Options.h"
#include "SABER/PathChecker.h"

#include <iostream>
#include <fstream>
#include <regex>

using namespace llvm;
using namespace std;
using namespace SVF;

static llvm::cl::opt<std::string> InputFilename(cl::Positional,
        llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

regex str_to_regex(string &str){
    str = regex_replace(str, regex("\\."), "\\.");
    str = regex_replace(str, regex("\\*"), "\\*");
    str = regex_replace(str, regex("\\+"), "\\+");
    str = regex_replace(str, regex("\\?"), "\\?");
    str = regex_replace(str, regex("\\^"), "\\^");
    str = regex_replace(str, regex("\\$"), "\\$");
    str = regex_replace(str, regex("\\|"), "\\|");

    str = regex_replace(str, regex("\\{\\}"), ".+");

    str = regex_replace(str, regex("\\{"), "\\{");
    str = regex_replace(str, regex("\\}"), "\\}");
    str = regex_replace(str, regex("\\("), "\\(");
    str = regex_replace(str, regex("\\)"), "\\)");
    str = regex_replace(str, regex("\\["), "\\[");
    str = regex_replace(str, regex("\\]"), "\\]");

    return regex(str);
}

int main(int argc, char ** argv)
{
    int arg_num = 0;
    char **arg_value = new char*[argc];
    vector<string> moduleNameVec;
    SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
                                "Source Sink Analysis using SVFG Node Numbers\n");
    
    if (Options::WriteAnder == "ir_annotator")
    {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }

    if (Options::DebugOnly != "")
    {
        llvm::DebugFlag = true;
        setCurrentDebugType(Options::DebugOnly.c_str());
    }

    PathChecker *pathChecker = new PathChecker();

    string str;

    // ! Parsing source file
    vector<InstNode> &srcVec = pathChecker->srcVec;

    ifstream src_stream(Options::SrcFile.getValue());
    if(!src_stream) {
        cerr << "Cannot open the source file\n";
        return false;
    }
    while(getline(src_stream, str)){
        if(str.size() > 0){
            srcVec.push_back(InstNode("", str));
        }
    }

    outs() << "\nParsing source nodes successed. include node size : " << pathChecker->srcVec.size() << "\n";

    // ! Parsing sink file
    vector<InstNode> &snkVec = pathChecker->snkVec;
    
    ifstream snk_stream(Options::SnkFile.getValue());
    string funcName;

    if(!snk_stream) {
        cerr << "Cannot open the sink file\n";
        return false;
    }
    while(getline(snk_stream, str)){
        if(str.size() > 0){
            if(str.rfind("Function : ") == 0){
                funcName = str.substr(11);
            } else{
                snkVec.push_back(InstNode(funcName, str));
            }
        }
    }

    for(auto &instNode : pathChecker->snkVec){
        if(pathChecker->funcToSnkVecMap.count(instNode.func)){
            pathChecker->funcToSnkVecMap[instNode.func].push_back(&instNode);
        }else{
            vector<InstNode *> instVec;
            instVec.push_back(&instNode);
            pathChecker->funcToSnkVecMap.insert(pair<string, vector<InstNode *>>(instNode.func, instVec));
        }
    }

    outs() << "\nParsing sink nodes successed. sink node size : " << pathChecker->snkVec.size() << "\n";

    // ! Parsing include file
    vector<InstNode> &incVec = pathChecker->incVec;

    ifstream inc_stream(Options::IncFile.getValue());
    if(!inc_stream) {
        cerr << "Cannot open the include file\n";
        return false;
    }
    while(getline(inc_stream, str)){
        if(str.size() > 0){
            incVec.push_back(InstNode("", str));
        }
    }

    outs() << "\nParsing include nodes successed. include node size : " << pathChecker->incVec.size() << "\n";
    
    // ! Parsing Add Edge file
    ifstream add_edge_stream(Options::AddEdgeFile.getValue());
    if(!add_edge_stream) {
        cerr << "Cannot open the add edge file\n";
        return false;
    }
    while(getline(add_edge_stream, str)){
        if(str.rfind("Store") == 0){
            AddEdge addEdge;
            InstNode *instNode;
            while(getline(add_edge_stream, str)){
                if(str.rfind("Load") == 0){
                    while(getline(add_edge_stream, str)){
                        if(str == ""){
                            break;
                        } else{
                            if(str.rfind("Function ") == 0){
                                instNode = new InstNode();
                                instNode->set_func(str.substr(9));
                                getline(add_edge_stream, str);
                                instNode->set_inst(str);
                                addEdge.toInstVec.push_back(*instNode);
                            }else{
                                instNode = new InstNode("", str);
                                addEdge.toInstVec.push_back(*instNode);
                            }
                        }
                    }
                } else {
                    if(str.rfind("Function ") == 0){
                        instNode = new InstNode();
                        instNode->set_func(str.substr(9));
                        getline(add_edge_stream, str);
                        instNode->set_inst(str);
                        addEdge.fromInstVec.push_back(*instNode);
                    }else{
                        instNode = new InstNode("", str);
                        addEdge.fromInstVec.push_back(*instNode);
                    }
                }
                if(str == ""){
                    break;
                }
            }
            pathChecker->addEdgeVec.push_back(addEdge);
        }
    }

    // int idxAddEdge = 0;
    // for(auto &addEdge : pathChecker->addEdgeVec){
    //     outs() << "addEdgeVec [" << idxAddEdge++ << "]\n";
    //     outs() << "\tfromInstVec : \n";
    //     for(auto it = addEdge.fromInstVec.begin();it!=addEdge.fromInstVec.end();it++){
    //         outs() << "\t\tinstNode : " << &(*it) << ", func : " << &(it->func) << ", inst : " << &(it->inst) << "\n";
    //     }
    //     outs() << "\ttoInstVec : \n";
    //     for(auto it = addEdge.toInstVec.begin();it!=addEdge.toInstVec.end();it++){
    //         outs() << "\t\tinstNode : " << &(*it) << ", func : " << &(it->func) << ", inst : " << &(it->inst) << "\n";
    //     }
    // }

    outs() << "\nParsing add edge successed. edge size : " << pathChecker->addEdgeVec.size() << "\n";

    for(auto &addEdge : pathChecker->addEdgeVec){
        for(auto &instNode : addEdge.fromInstVec){
            if(pathChecker->funcToInstNodeVecMap.count(instNode.func)){
                pathChecker->funcToInstNodeVecMap[instNode.func].push_back(&instNode);
            }else{
                vector<InstNode *> instVec;
                instVec.push_back(&instNode);
                pathChecker->funcToInstNodeVecMap.insert(pair<string, vector<InstNode *>>(instNode.func, instVec));
            }
        }
        for(auto &instNode : addEdge.toInstVec){
            if(pathChecker->funcToInstNodeVecMap.count(instNode.func)){
                pathChecker->funcToInstNodeVecMap[instNode.func].push_back(&instNode);
            }else{
                vector<InstNode *> instVec;
                instVec.push_back(&instNode);
                pathChecker->funcToInstNodeVecMap.insert(pair<string, vector<InstNode *>>(instNode.func, instVec));
            }
        }
    }

    // for(auto it : pathChecker->funcToInstNodeVecMap){
    // for(auto it = pathChecker->funcToInstNodeVecMap.begin();it!=pathChecker->funcToInstNodeVecMap.end();it++){
    //     outs() << "key : " << it->first << "\n";
    //     outs() << "value : \n";
    //     for(auto instNode=it->second.begin();instNode!=it->second.end();instNode++){
    //         outs() << "\t\tfunc : " << &(*instNode)->func << ", inst : " << &((*instNode)->inst) << "\n";
    //     }
    // }

    outs() << "\nMapping funciton name to instNode successed. map size : " << pathChecker->funcToInstNodeVecMap.size() << "\n";

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);
    svfModule->buildSymbolTableInfo();

    ofstream svfModuleInstFile;
    svfModuleInstFile.open("svfModule.bc");
    for(auto llvm_func = svfModule->llvmFunBegin(); llvm_func != svfModule->llvmFunEnd(); llvm_func++){
        Function &F = **llvm_func;
        svfModuleInstFile << "Function : " << F.getName().str() << "\n";
        for(llvm::BasicBlock &BB : F){
            for(llvm::Instruction &IN : BB){
                string inst_str;
                llvm::raw_string_ostream inst_stream(inst_str);
                inst_stream << IN;

                svfModuleInstFile << inst_stream.str() << "\n";
            }
        }
    }
    svfModuleInstFile.close();

    pathChecker->runOnModule(svfModule);

    // LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}


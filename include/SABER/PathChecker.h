//===- PathChecker.h -- Detecting memory leaks--------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2017>  <Yulei Sui>
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
//===----------------------------------------------------------------------===//

/*
 * PathChecker.h
 *
 *  Created on: Apr 21, 2022
 *      Author: acorn421
 */

#ifndef PATHCHECKER_H_
#define PATHCHECKER_H_

#include "SABER/SrcSnkDDA.h"
#include "SABER/SaberCheckerAPI.h"
#include <regex>

using namespace std;

namespace SVF
{

class IRNode {
public:
    IRNode(string _func) : func(_func) {}
    ~IRNode() {}
    string func;
    vector<regex> instVec;

    bool operator==(const llvm::StringRef& _func) {
        return func == _func;
    }

    bool operator==(const IRNode& _right) {
        return func == _right.func;
    }
};

class InstNode {
public:
    InstNode() {}
    InstNode(string _func, string _inst) {
        func = _func;

        str = _inst;
        
        _inst = regex_replace(_inst, regex("\\."), "\\.");
        _inst = regex_replace(_inst, regex("\\*"), "\\*");
        _inst = regex_replace(_inst, regex("\\+"), "\\+");
        _inst = regex_replace(_inst, regex("\\?"), "\\?");
        _inst = regex_replace(_inst, regex("\\^"), "\\^");
        _inst = regex_replace(_inst, regex("\\$"), "\\$");
        _inst = regex_replace(_inst, regex("\\|"), "\\|");

        _inst = regex_replace(_inst, regex("\\{\\}"), ".+");

        _inst = regex_replace(_inst, regex("\\{"), "\\{");
        _inst = regex_replace(_inst, regex("\\}"), "\\}");
        _inst = regex_replace(_inst, regex("\\("), "\\(");
        _inst = regex_replace(_inst, regex("\\)"), "\\)");
        _inst = regex_replace(_inst, regex("\\["), "\\[");
        _inst = regex_replace(_inst, regex("\\]"), "\\]");

        inst = regex(_inst);
    }
    ~InstNode() {}

    string func;
    regex inst;
    string str;
    vector<SVFGNode *> nodeVec;

    void set_func(string _func){
        func = _func;
    }

    void set_inst(string _inst){
        str = _inst;

        _inst = regex_replace(_inst, regex("\\."), "\\.");
        _inst = regex_replace(_inst, regex("\\*"), "\\*");
        _inst = regex_replace(_inst, regex("\\+"), "\\+");
        _inst = regex_replace(_inst, regex("\\?"), "\\?");
        _inst = regex_replace(_inst, regex("\\^"), "\\^");
        _inst = regex_replace(_inst, regex("\\$"), "\\$");
        _inst = regex_replace(_inst, regex("\\|"), "\\|");

        _inst = regex_replace(_inst, regex("\\{\\}"), ".+");

        _inst = regex_replace(_inst, regex("\\{"), "\\{");
        _inst = regex_replace(_inst, regex("\\}"), "\\}");
        _inst = regex_replace(_inst, regex("\\("), "\\(");
        _inst = regex_replace(_inst, regex("\\)"), "\\)");
        _inst = regex_replace(_inst, regex("\\["), "\\[");
        _inst = regex_replace(_inst, regex("\\]"), "\\]");

        inst = regex(_inst);
    }

    // bool operator==(const std::pair<string, string>& _ir) {
    //     return func == _ir.first && regex_match(_ir.second, inst);
    // }
};

class AddEdge {
public:
    AddEdge() {}
    // AddEdge(InstNode _fromInst, InstNode _toInst) : fromInst(_fromInst), toInst(_toInst) {}
    // AddEdge(string _from_func, string _from_inst, string _to_func, string _to_inst) {
    //     fromInst.set_func(_from_func);
    //     fromInst.set_inst(_from_inst);
    //     toInst.set_func(_to_func);
    //     toInst.set_inst(_to_inst);
    // }
    ~AddEdge() {}

    vector<InstNode> fromInstVec;
    vector<InstNode> toInstVec;
};

class PathChecker : public SrcSnkDDA
{

public:
    typedef Map<const SVFGNode*,const CallBlockNode*> SVFGNodeToCSIDMap;
    typedef FIFOWorkList<const CallBlockNode*> CSWorkList;
    typedef ProgSlice::VFWorkList WorkList;
    typedef NodeBS SVFGNodeBS;

    /// Constructor
    PathChecker()
    {
    }
    /// Destructor
    virtual ~PathChecker()
    {
    }

    /// We start from here
    virtual bool runOnModule(SVFModule* module)
    {
        /// start analysis
        analyze(module);
        return false;
    }

    // redefine from SrcSnkDDA, LeakChecker
    virtual void initSrcSnk(SVFModule* svfModule);
    virtual void initSrcs() override;
    virtual void initSnks() override;
    virtual void initialize(SVFModule* module) override;
    virtual void analyze(SVFModule* module) override;
    virtual void finalize() override;

    // redefine from CFLSolver
    virtual void forwardTraverse(DPIm& it) override;
    virtual void backwardTraverse(DPIm& it) override;
    virtual void FWProcessCurNode(const DPIm&) override;
    virtual void BWProcessCurNode(const DPIm&) override;
    virtual void FWProcessOutgoingEdge(const DPIm& item, GEDGE* edge) override;
    virtual void BWProcessIncomingEdge(const DPIm& item, GEDGE* edge) override;
    
    /// Whether the function is a heap allocator/reallocator (allocate memory)
    virtual inline bool isSourceLikeFun(const SVFFunction* fun) override
    {
        return SaberCheckerAPI::getCheckerAPI()->isMemAlloc(fun);
    }
    /// Whether the function is a heap deallocator (free/release memory)
    virtual inline bool isSinkLikeFun(const SVFFunction* fun) override
    {
        return SaberCheckerAPI::getCheckerAPI()->isMemDealloc(fun);
    }
    //@}

    // vector<regex> srcVec;
    vector<InstNode> srcVec;
    // vector<IRNode> snkVec;
    vector<InstNode> snkVec;
    // vector<string> incVec;
    vector<InstNode> incVec;
    // vector<int> srcIdVec;
    // vector<int> snkIdVec;
    // vector<int> incIdVec;
    // vector<SVFGNode*> srcSVFGNodeVec;
    // vector<SVFGNode*> snkSVFGNodeVec;
    // vector<SVFGNode*> incSVFGNodeVec;

    vector<AddEdge> addEdgeVec;
    map<string, vector<InstNode *>> funcToInstNodeVecMap;
    map<string, vector<InstNode *>> funcToSnkVecMap;

    PAG* pag;
    SVFG* svfg;

protected:
    virtual void reportBug(ProgSlice* slice) override;
    /// Record a source to its callsite
    //@{
    inline void addSrcToCSID(const SVFGNode* src, const CallBlockNode* cs)
    {
        srcToCSIDMap[src] = cs;
    }
    inline const CallBlockNode* getSrcCSID(const SVFGNode* src)
    {
        SVFGNodeToCSIDMap::iterator it =srcToCSIDMap.find(src);
        assert(it!=srcToCSIDMap.end() && "source node not at a callsite??");
        return it->second;
    }
    //@}
private:
    SVFGNodeToCSIDMap srcToCSIDMap;
};

} // End namespace SVF

#endif /* PATHCHECKER_H_ */

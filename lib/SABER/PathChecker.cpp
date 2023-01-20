//===- PathChecker.cpp -- Memory leak detector ------------------------------//
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
 * PathChecker.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: Yulei Sui
 */

#include "Util/Options.h"
#include "SVF-FE/LLVMUtil.h"
#include "SABER/PathChecker.h"
#include "SVF-FE/PAGBuilder.h"
#include "WPA/Andersen.h"

#include <iostream>
#include <fstream>

using namespace SVF;
using namespace SVFUtil;


void PathChecker::initialize(SVFModule* module)
{
	PAGBuilder builder;
	pag = builder.build(module);

    AndersenWaveDiff* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
    // svfg =  memSSA.buildPTROnlySVFG(ander);
    
    SVFGBuilder svfBuilder(true);
    svfg = svfBuilder.buildFullSVFG(ander);
    setGraph(svfg);

    //svfg =  memSSA.buildFullSVFG(ander);
    //setGraph(memSSA.getSVFG());


    ptaCallGraph = ander->getPTACallGraph();
    //AndersenWaveDiff::releaseAndersenWaveDiff();
    /// allocate control-flow graph branch conditions
    getPathAllocator()->allocate(getPAG()->getModule());

    //llvm::outs() << "Printing original SVF graph to original_svfg.dot\n";
    //memSSA.getSVFG()->dump("original_svfg", true);

    initSrcSnk(module);
}

/*!
 * Initialize sources and sinks
 */
void PathChecker::initSrcSnk(SVFModule* svfModule)
{
    for(auto llvm_func = svfModule->llvmFunBegin(); llvm_func != svfModule->llvmFunEnd(); llvm_func++){
        Function &F = **llvm_func;
        map<string, vector<InstNode *>>::iterator iterAddEdge, iterSnkNode;

        for(llvm::BasicBlock &BB : F){
            StringRef funcName = F.getName();
            iterSnkNode = find_if(funcToSnkVecMap.begin(), funcToSnkVecMap.end(), [funcName](pair<string, vector<InstNode *>> const& elem){
                return elem.first == funcName;
            });
            iterAddEdge = find_if(funcToInstNodeVecMap.begin(), funcToInstNodeVecMap.end(), [funcName](pair<string, vector<InstNode *>> const& elem){
                return elem.first == funcName;
            });

            for(llvm::Instruction &IN : BB){
                string inst_str;
                llvm::raw_string_ostream inst_stream(inst_str);
                inst_stream << IN;

                int srcIdx = 0;
                for(InstNode &srcInst : srcVec){
                    if(regex_match(inst_stream.str(), srcInst.inst)){
                        auto svfgNodeSet = svfg->fromValue(&IN);

                        if(svfgNodeSet.empty()){
                            DBOUT(DPathChecker, llvm::outs() << "[WARN] Cannot get Source SVFGNode from LLVM Value :" << "\n");
                            DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                        }else{
                            for(auto &_svfgNode : svfgNodeSet){
                                SVFGNode* svfgNode = const_cast<SVFGNode *>(_svfgNode);
                                srcInst.nodeVec.push_back(svfgNode);

                                DBOUT(DPathChecker, llvm::outs() << "Source SVFGNode Id : " << svfgNode->getId() << "\n");
                                DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                            }
                        }
                    }
                    srcIdx++;
                }
                
                int incIdx = 0;
                for(InstNode &incInst : incVec){
                    if(regex_match(inst_stream.str(), incInst.inst)){
                        auto svfgNodeSet = svfg->fromValue(&IN);

                        if(svfgNodeSet.empty()){
                            DBOUT(DPathChecker, llvm::outs() << "[WARN] Cannot get Include SVFGNode from LLVM Value :" << "\n");
                            DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                        }else{
                            for(auto &_svfgNode : svfgNodeSet){
                                SVFGNode* svfgNode = const_cast<SVFGNode *>(_svfgNode);
                                incInst.nodeVec.push_back(svfgNode);

                                DBOUT(DPathChecker, llvm::outs() << "Include SVFGNode Id : " << svfgNode->getId() << "\n");
                                DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                            }
                        }
                    }
                    incIdx++;
                }
                
                if(iterSnkNode != funcToSnkVecMap.end()){
                    for(auto &snkInst : iterSnkNode->second){
                        if(regex_match(inst_stream.str(), snkInst->inst)){
                            auto svfgNodeSet = svfg->fromValue(&IN);

                            if(svfgNodeSet.empty()){
                                DBOUT(DPathChecker, llvm::outs() << "[WARN] Cannot get Sink SVFGNode from LLVM Value :" << "\n");
                                DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                            }else{
                                for(auto &_svfgNode : svfgNodeSet){
                                    SVFGNode* svfgNode = const_cast<SVFGNode *>(_svfgNode);
                                    snkInst->nodeVec.push_back(svfgNode);
                                    InstNode _snkInst = *snkInst;
                                    // chkSnkArr[distance(snkVec.begin(), find(snkVec.begin(), snkVec.end(), _snkInst))]++;

                                    DBOUT(DPathChecker, llvm::outs() << "Sink SVFGNode Id : " << svfgNode->getId() << "\n");
                                    DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                                }
                            }
                        }
                    }
                }

                if(iterAddEdge != funcToInstNodeVecMap.end() && funcName != "") {
                    for(auto &iterInstNode : iterAddEdge->second) {
                        if(regex_match(inst_stream.str(), iterInstNode->inst)){
                            auto svfgNodeSet = svfg->fromValue(&IN);

                            if(svfgNodeSet.empty()){
                                DBOUT(DPathChecker, llvm::outs() << "[WARN] Cannot get AddEdge SVFGNode from LLVM Value" << "\n");
                                DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                            }else{
                                for(auto &_svfgNode : svfgNodeSet){
                                    SVFGNode* svfgNode = const_cast<SVFGNode *>(_svfgNode);
                                    iterInstNode->nodeVec.push_back(svfgNode);

                                    DBOUT(DPathChecker, llvm::outs() << "AddEdge SVFGNode Id : " << svfgNode->getId() << "\n");
                                    DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                                }
                            }
                        }
                    }
                }

                for(auto &iterInstNode : funcToInstNodeVecMap[""]) {
                    if(regex_match(inst_stream.str(), iterInstNode->inst)){
                        auto svfgNodeSet = svfg->fromValue(&IN);

                        if(svfgNodeSet.empty()){
                            DBOUT(DPathChecker, llvm::outs() << "[WARN] Cannot get AddEdge SVFGNode from LLVM Value : " << "\n");
                            DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                        }else{
                            for(auto &_svfgNode : svfgNodeSet){
                                SVFGNode* svfgNode = const_cast<SVFGNode *>(_svfgNode);
                                iterInstNode->nodeVec.push_back(svfgNode);

                                DBOUT(DPathChecker, llvm::outs() << "AddEdge SVFGNode Id : " << svfgNode->getId() << "\n");
                                DBOUT(DPathChecker, llvm::outs() << "\t" << inst_stream.str() << "\n");
                            }
                        }
                    }
                }
            }
        }

        // Remove indirect VFGEdge from StoreSVFGNode
        auto svfFunction = svfModule->getSVFFunction(*llvm_func);
        SVFGNode::GEdgeSetTy dirEdgeSet;
        
        if(svfg->hasVFGNodes(svfFunction)){
            for(auto it = svfg->getVFGNodeBegin(svfFunction);it!=svfg->getVFGNodeEnd(svfFunction);it++){
                auto node = *it;
                if(SVFUtil::isa<StoreSVFGNode>(node) && node->hasOutgoingEdge()){
                    for(auto eit = (*it)->OutEdgeBegin();eit!=(*it)->OutEdgeEnd();eit++){
                        auto edge = *eit;
                        outs() << node->toString() << "\n";
                        outs() << edge->toString() << "\n";
                        if(edge->isIndirectVFGEdge()){
                            dirEdgeSet.insert(edge);
                            // svfg->removeVFGEdge(edge);
                        }
                    }
                }
            }
        }
        

        for(auto it = dirEdgeSet.begin();it!=dirEdgeSet.end();it++){
            auto edge = *it;
            svfg->removeVFGEdge(edge);
        }
    }

    llvm::outs() << "Finished to find SVFG Nodes\n";

    for(auto &srcInst : srcVec){
        for(auto &srcNode : srcInst.nodeVec){
            addToSources(srcNode);
        }
    }

    for(auto &snkInst : snkVec){
        for(auto &snkNode : snkInst.nodeVec){
            addToSinks(snkNode);
        }
    }

    llvm::outs() << "Finished to initialize sources, sinks, and includes\n";

    for(auto &addEdgeNode : addEdgeVec){
        for(auto &fromInstNode : addEdgeNode.fromInstVec){
            for(auto &toInstNode : addEdgeNode.toInstVec){
                if(fromInstNode.nodeVec.size() == 0 || toInstNode.nodeVec.size() == 0){
                    llvm::outs() << "\t[WARN] Cannot find AddEdge SVFGNode from bitcode\n";
                    if(fromInstNode.nodeVec.size() == 0){
                        llvm::outs() << "\t\tfromInstNode size : " << fromInstNode.nodeVec.size() << ", str : " << fromInstNode.str << "\n";
                    }else{
                        llvm::outs() << "\t\ttoInstNode size : " << toInstNode.nodeVec.size() << ", str : " << toInstNode.str << "\n";
                    }
                }else {
                    for(auto &fromSVFGNode : fromInstNode.nodeVec){
                        for(auto &toSVFGNode : toInstNode.nodeVec){
                            if(fromSVFGNode->getId() != toSVFGNode->getId()){
                                if(svfg->hasIntraVFGEdge(fromSVFGNode, toSVFGNode, VFGEdge::IntraDirectVF) == nullptr){
                                    IntraDirSVFGEdge* directEdge = new IntraDirSVFGEdge(fromSVFGNode, toSVFGNode);
                                    svfg->addSVFGEdge(directEdge);
                                }
                            }
                            // svfg->addIntraDirectVFEdge(fromSVFGNode->getId(), toSVFGNode->getId());
                            // IntraDirSVFGEdge* directEdge = new IntraDirSVFGEdge(fromSVFGNode, toSVFGNode);
                            // svfg->addSVFGEdge(directEdge);
                            // svfg->addIntraIndirectVFEdge();
                        }
                    }
                }
            }
        }
    }

    for(auto &srcInst : srcVec){
        if(srcInst.nodeVec.size() == 0){
            llvm::outs() << "\t[WARN] Cannot find Source SVFGNode from bitcode\n";
            llvm::outs() << "\t\tstr : " << srcInst.str << "\n";
        }
    }

    for(auto &snkInst : snkVec){
        if(snkInst.nodeVec.size() == 0){
            llvm::outs() << "\t[WARN] Cannot find Sink SVFGNode from bitcode\n";
            llvm::outs() << "\t\tstr : " << snkInst.str << "\n";
        }
    }

    for(auto &incInst : incVec){
        if(incInst.nodeVec.size() == 0){
            llvm::outs() << "\t[WARN] Cannot find Include SVFGNode from bitcode\n";
            llvm::outs() << "\t\tstr : " << incInst.str << "\n";
        }
    }

    llvm::outs() << "Printing addEdge SVF graph to add_edge_svfg.dot\n";
    svfg->dump("add_edge_svfg", true);

    llvm::outs() << "Finished to add edge\n";
}

void PathChecker::initSrcs() {
    return;
}

void PathChecker::initSnks() {
    return;
}

void PathChecker::reportBug(ProgSlice* slice) {
    return;
}

void PathChecker::analyze(SVFModule* module)
{
    ContextCond empty_cxt;
    DPIm empty_item(0, empty_cxt);

    initialize(module);

    ContextCond::setMaxCxtLen(Options::CxtLimit);

    // setCurSlice(*(srcVec.begin()->nodeVec.begin()));

    for(auto &srcInst : srcVec){
        for(auto &srcNode : srcInst.nodeVec){
            if(!getCurSlice()) {
                setCurSlice(srcNode);
            }
            SVFGNode *svfgNode = srcNode;
            ContextCond cxt;
            DPIm item(svfgNode->getId(), cxt);

            pushIntoWorklist(item);
        }
    }

    forwardTraverse(empty_item);

    for (SVFGNodeSetIter sit = getCurSlice()->sinksBegin(), esit =
                        getCurSlice()->sinksEnd(); sit != esit; ++sit)
    {
        ContextCond cxt;
        DPIm item((*sit)->getId(),cxt);

        pushIntoWorklist(item);
    }

    backwardTraverse(empty_item);
    
    finalize();
}

void PathChecker::forwardTraverse(DPIm& it)
{
    while (!isWorklistEmpty())
    {
        DPIm item = popFromWorklist();
        FWProcessCurNode(item);

        GNODE* v = getNode(getNodeIDFromItem(item));
        child_iterator EI = GTraits::child_begin(v);
        child_iterator EE = GTraits::child_end(v);
        for (; EI != EE; ++EI)
        {
            FWProcessOutgoingEdge(item,*(EI.getCurrent()) );
        }
    }
}

void PathChecker::backwardTraverse(DPIm& it)
{
    while (!isWorklistEmpty())
    {
        DPIm item = popFromWorklist();
        BWProcessCurNode(item);

        GNODE* v = getNode(getNodeIDFromItem(item));
        inv_child_iterator EI = InvGTraits::child_begin(v);
        inv_child_iterator EE = InvGTraits::child_end(v);
        for (; EI != EE; ++EI)
        {
            BWProcessIncomingEdge(item,*(EI.getCurrent()) );
        }
    }
}

void PathChecker::FWProcessCurNode(const DPIm& item)
{
    const SVFGNode* node = getNode(item.getCurNodeID());
    if(isSink(node))
    {
        addSinkToCurSlice(node);
        _curSlice->setPartialReachable();
        addToCurForwardSlice(node);
        llvm::outs() << "\t\tPath was found from source to sink (ID : " << node->getId() << ")\n";
    }
    else
        addToCurForwardSlice(node);
}

void PathChecker::BWProcessCurNode(const DPIm& item)
{
    const SVFGNode* node = getNode(item.getCurNodeID());
    if(isInCurForwardSlice(node))
    {
        addToCurBackwardSlice(node);
    }
}

void PathChecker::FWProcessOutgoingEdge(const DPIm& item, SVFGEdge* edge)
{
    DBOUT(DSaber,outs() << "\n##processing source: " << getCurSlice()->getSource()->getId() <<" forward propagate from (" << edge->getSrcID());

    // for indirect SVFGEdge, the propagation should follow the def-use chains
    // points-to on the edge indicate whether the object of source node can be propagated

    const SVFGNode* dstNode = edge->getDstNode();
    DPIm newItem(dstNode->getId(),item.getContexts());

    /// handle globals here
    // if(isGlobalSVFGNode(dstNode) || getCurSlice()->isReachGlobal())
    // {
    //     getCurSlice()->setReachGlobal();
    //     return;
    // }


    /// perform context sensitive reachability
    // push context for calling
    if (edge->isCallVFGEdge())
    {
        CallSiteID csId = 0;
        if(const CallDirSVFGEdge* callEdge = SVFUtil::dyn_cast<CallDirSVFGEdge>(edge))
            csId = callEdge->getCallSiteId();
        else
            csId = SVFUtil::cast<CallIndSVFGEdge>(edge)->getCallSiteId();

        newItem.pushContext(csId);
        DBOUT(DSaber, outs() << " push cxt [" << csId << "] ");
    }
    // match context for return
    else if (edge->isRetVFGEdge())
    {
        CallSiteID csId = 0;
        if(const RetDirSVFGEdge* callEdge = SVFUtil::dyn_cast<RetDirSVFGEdge>(edge))
            csId = callEdge->getCallSiteId();
        else
            csId = SVFUtil::cast<RetIndSVFGEdge>(edge)->getCallSiteId();

        if (newItem.matchContext(csId) == false)
        {
            DBOUT(DSaber, outs() << "-|-\n");
            return;
        }
        DBOUT(DSaber, outs() << " pop cxt [" << csId << "] ");
    }

    /// whether this dstNode has been visited or not
    if(forwardVisited(dstNode,newItem))
    {
        DBOUT(DSaber,outs() << " node "<< dstNode->getId() <<" has been visited\n");
        return;
    }
    else
        addForwardVisited(dstNode, newItem);

    if(pushIntoWorklist(newItem))
        DBOUT(DSaber,outs() << " --> " << edge->getDstID() << ", cxt size: " << newItem.getContexts().cxtSize() <<")\n");

}

void PathChecker::BWProcessIncomingEdge(const DPIm&, SVFGEdge* edge)
{
    DBOUT(DSaber,outs() << "backward propagate from (" << edge->getDstID() << " --> " << edge->getSrcID() << ")\n");
    const SVFGNode* srcNode = edge->getSrcNode();
    if(backwardVisited(srcNode))
        return;
    else
        addBackwardVisited(srcNode);

    ContextCond cxt;
    DPIm newItem(srcNode->getId(), cxt);
    pushIntoWorklist(newItem);
}

void PathChecker::finalize()
{
    llvm::outs() << "Finalizing...\n";

    if(getCurSlice()->getBackwardSliceSize() == 0){
        llvm::outs() << "[WARN] Nothing reacahble from sources to sinks\n";
    } else{
        SVFG *new_svfg = new SVFG();

        llvm::outs() << "ReachableSet : ";
        for(SVFGNodeSetIter iter = getCurSlice()->backwardSliceBegin();iter!=getCurSlice()->backwardSliceEnd();iter++){
            SVFGNode *slice = const_cast<SVFGNode *>(*iter);
            llvm::outs() << slice->getId() << " ";
            new_svfg->addSVFGNode(const_cast<SVFGNode *>(slice), const_cast<ICFGNode *>(slice->getICFGNode()));
        }
        llvm::outs() << "\nChecking if include nodes are in reachableSet.\n";

        // int id = 0;
        // for(auto &incInst : incVec){
        //     for(auto &incNode : incInst.nodeVec){
        //         llvm::outs() << "inc id " << id << " : " << (getCurSlice()->inBackwardSlice((const SVFGNode *)incNode) ? "true" : "false") << "\n";
        //         id++;
        //     }
        // }

        llvm::outs() << "\tSize of ReachableSet : " << getCurSlice()->getBackwardSliceSize() << ", new_svfg : " << new_svfg->nodeNum << "\n";

        llvm::outs() << "\tPrint slice graph to dot file\n";
        new_svfg->dump("reachableNodes", true);

        delete new_svfg;
    }
}

//===- GraphPrinter.h -- Print Generic Graph---------------------//
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
 * GraphPrinterSlice.h
 *
 *  Created on: 21.April.,2022
 *      Author: acorn421
 */

#ifndef INCLUDE_UTIL_GRAPHPRINTER_SLICE_H_
#define INCLUDE_UTIL_GRAPHPRINTER_SLICE_H_

#include <system_error>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/FileSystem.h>		// for file open flag
#include <llvm/ADT/GraphTraits.h>
#include <llvm/Support/GraphWriter.h>
#include "SABER/ProgSlice.h"

using namespace SVF;

namespace llvm
{

/*
 * Dump and print the graph for debugging
 */
template<typename GraphType>
class GraphWriterSlice : public GraphWriter
{
public:
    static ProgSlice::SVFGNodeSet& slice;

    void writeNode(NodeRef Node) override {
        std::string NodeAttributes = DTraits.getNodeAttributes(Node, G);

        O << "\tNode" << static_cast<const void*>(Node) << " [shape=record,";
        if (!NodeAttributes.empty()) O << NodeAttributes << ",";
        O << "label=\"{";

        if (!DTraits.renderGraphFromBottomUp()) {
        O << DOT::EscapeString(DTraits.getNodeLabel(Node, G));

        // If we should include the address of the node in the label, do so now.
        std::string Id = DTraits.getNodeIdentifierLabel(Node, G);
        if (!Id.empty())
            O << "|" << DOT::EscapeString(Id);

        std::string NodeDesc = DTraits.getNodeDescription(Node, G);
        if (!NodeDesc.empty())
            O << "|" << DOT::EscapeString(NodeDesc);
        }

        std::string edgeSourceLabels;
        raw_string_ostream EdgeSourceLabels(edgeSourceLabels);
        bool hasEdgeSourceLabels = getEdgeSourceLabels(EdgeSourceLabels, Node);

        if (hasEdgeSourceLabels) {
        if (!DTraits.renderGraphFromBottomUp()) O << "|";

        O << "{" << EdgeSourceLabels.str() << "}";

        if (DTraits.renderGraphFromBottomUp()) O << "|";
        }

        if (DTraits.renderGraphFromBottomUp()) {
        O << DOT::EscapeString(DTraits.getNodeLabel(Node, G));

        // If we should include the address of the node in the label, do so now.
        std::string Id = DTraits.getNodeIdentifierLabel(Node, G);
        if (!Id.empty())
            O << "|" << DOT::EscapeString(Id);

        std::string NodeDesc = DTraits.getNodeDescription(Node, G);
        if (!NodeDesc.empty())
            O << "|" << DOT::EscapeString(NodeDesc);
        }

        if (DTraits.hasEdgeDestLabels()) {
        O << "|{";

        unsigned i = 0, e = DTraits.numEdgeDestLabels(Node);
        for (; i != e && i != 64; ++i) {
            if (i) O << "|";
            O << "<d" << i << ">"
            << DOT::EscapeString(DTraits.getEdgeDestLabel(Node, i));
        }

        if (i != e)
            O << "|<d64>truncated...";
        O << "}";
        }

        O << "}\"];\n";   // Finish printing the "node" line

        // Output all of the edges now
        child_iterator EI = GTraits::child_begin(Node);
        child_iterator EE = GTraits::child_end(Node);
        for (unsigned i = 0; EI != EE && i != 64; ++EI, ++i)
        if (!DTraits.isNodeHidden(*EI, G))
            writeEdge(Node, i, EI);
        for (; EI != EE; ++EI)
        if (!DTraits.isNodeHidden(*EI, G))
            writeEdge(Node, 64, EI);
    }
};

class GraphPrinterSlice
{

public:
    GraphPrinterSlice()
    {
    }

    /*!
     *  Write the graph into dot file for debugging purpose
     */
    template<class GraphType>
    static void WriteGraphToFile(llvm::raw_ostream &O,
                                 const std::string &GraphName, const GraphType &GT, ProgSlice::SVFGNodeSet& slice, bool simple = false)
    {
        // Filename of the output dot file
        std::string Filename = GraphName + ".dot";
        O << "Writing '" << Filename << "'...";
        std::error_code ErrInfo;
        llvm::ToolOutputFile F(Filename.c_str(), ErrInfo, llvm::sys::fs::OF_None);

        GraphWriterSlice::slice = slice;

        if (!ErrInfo)
        {
            // dump the ValueFlowGraph here
            GraphWriterSlice::WriteGraph(F.os(), GT, simple);
            F.os().close();
            if (!F.os().has_error())
            {
                O << "\n";
                F.keep();
                return;
            }
        }
        O << "  error opening file for writing!\n";
        F.os().clear_error();
    }

    /*!
     * Print the graph to command line
     */
    template<class GraphType>
    static void PrintGraph(llvm::raw_ostream &O, const std::string &GraphName,
                           const GraphType &GT)
    {
        ///Define the GTraits and node iterator for printing
        typedef llvm::GraphTraits<GraphType> GTraits;

        typedef typename GTraits::NodeRef NodeRef;
        typedef typename GTraits::nodes_iterator node_iterator;
        typedef typename GTraits::ChildIteratorType child_iterator;

        O << "Printing VFG Graph" << "'...\n";
        // Print each node name and its edges
        node_iterator I = GTraits::nodes_begin(GT);
        node_iterator E = GTraits::nodes_end(GT);
        for (; I != E; ++I)
        {
            NodeRef *Node = *I;
            O << "node :" << Node << "'\n";
            child_iterator EI = GTraits::child_begin(Node);
            child_iterator EE = GTraits::child_end(Node);
            for (unsigned i = 0; EI != EE && i != 64; ++EI, ++i)
            {
                O << "child :" << *EI << "'\n";
            }
        }
    }
};

} // End namespace llvm



#endif /* INCLUDE_UTIL_GRAPHPRINTER_SLICE_H_ */

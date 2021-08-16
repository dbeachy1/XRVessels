/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

//-------------------------------------------------------------------------
// ParserTree.h : definition of ParserTree class.
//-------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include "ParserTreeNode.h"

class ParserTree
{
public:
    ParserTree() 
    { 
        // build our root node
        m_rootParserTreeNode = new ParserTreeNode(nullptr, -1);

        // allocate an autocompletionstate object we will pass to our parser
        m_pAutocompletionState = ParserTreeNode::AllocateNewAutocompletionState();
    }

    virtual ~ParserTree() 
    { 
        delete m_rootParserTreeNode;  // recursively delete all nodes
        delete m_pAutocompletionState;
    }

    void AddTopLevelNode(ParserTreeNode *pNode)
    {
        m_rootParserTreeNode->AddChild(pNode);
    }

    bool AutoComplete(CString &csCommand, const bool direction) const
    {
        return m_rootParserTreeNode->AutoComplete(csCommand, m_pAutocompletionState, direction);
    }

    int GetAvailableArgumentsForCommand(CString &csCommand, vector<CString> &argsOut) const 
    { 
        return m_rootParserTreeNode->GetAvailableArgumentsForCommand(csCommand, argsOut); 
    }

    bool Parse(const char *pCommand, CString &statusOut) const
    {
        return m_rootParserTreeNode->Parse(pCommand, statusOut);
    }

    void ResetAutocompletionState()
    {
        ParserTreeNode::ResetAutocompletionState(m_pAutocompletionState);
    }

    void BuildCommandHelpTree(CString &csOut)
    {
        m_rootParserTreeNode->BuildCommandHelpTree(0, csOut);
    }

protected:
    ParserTreeNode *m_rootParserTreeNode;  

    // maintains autompletion state for our parser tree
    ParserTreeNode::AUTOCOMPLETION_STATE *m_pAutocompletionState;
};
//-------------------------------------------------------------------------
// ParserTree.h : definition of ParserTree class.
//
// Copyright 2010-2016 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
// 
// NOTE: You may not redistribute this file nor use it in any other project without
// express consent from the author.
//
// http://www.alteaaerospace.com
// mailto:dbeachy@speakeasy.net
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
        m_rootParserTreeNode = new ParserTreeNode(NULL, -1);

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
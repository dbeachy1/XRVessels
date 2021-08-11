//-------------------------------------------------------------------------
// ParserTreeNode.h : definition of ParserTreeNode class.
//
// Copyright 2010-2016 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
// 
// NOTE: You may not redistribute this file nor use it in any other project without
// express consent from the author.
//
// http://www.alteaaerospace.com
// mailto:doug.beachy@outlook.com
//-------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <vector>
#include <atlstr.h>

using namespace std;

class ParserTreeNode
{
public:
    typedef void * AUTOCOMPLETION_STATE;   // client-visible autocomopletion data type

    // interface that must be implemented by each NodeData subclass
    struct NodeData
    {
        virtual NodeData *Clone() const = 0;   // deep-clone this objet
    };

    // This abstract class defines a callback object that is invoked for leaf node; typically these
    // leaf node handlers will parse any remainging text (e.g., integers or doubles) and then 
    // perform work with those values.
    class LeafHandler
    {
    public:
        // pTreeNode = ParserTreeNode that called this leaf handler; e.g., "ThrottleLevel" in chain Set->LeftMain->ThrottleLevel #0.56
        // remainingArgv = remaining text arguments (typically number values)
        // statusOut = CString to which status message will be written
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut) = 0;

        // Returns a help string describing available valid arguments for this leaf node; e.g., "<double>"
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const = 0;

        // Returns an array of valid autocompletion string values for the first parameter; default is null
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { return nullptr; }

        // static utility methods
        static bool ParseValidatedDouble(const char *pStr, double &dblOut, const double min, const double max, CString *pCSErrorMsgOut);
        static bool ParseValidatedBool(const char *pStr, bool &boolOut, CString *pCSErrorMsgOut);
        static bool ParseValidatedInt(const char *pStr, int &intOut, const int min, const int max, CString *pCSErrorMsgOut);
        
        static bool ParseDouble(const char *pStr, double &dblOut);
        static bool ParseBool(const char *pStr, bool &boolOut);
        static bool ParseInt(const char *pStr, int &intOut);
    };

    ParserTreeNode(const char *pNodeText, const int nodeGroup, const NodeData *pNodeData = nullptr, LeafHandler *pCallback = nullptr);
    virtual ~ParserTreeNode();

    const CString *GetNodeText() const { return m_pCSNodeText; }  // e.g., "Set", "Main", etc.
    const NodeData *GetNodeData() const { return m_pNodeData; }   // e.g., XRE_MainLeft, double *, etc.
    int GetNodeGroup() const { return m_nodeGroup; } 
    void AddChild(ParserTreeNode *pChildNode);
    void SetParentNode(ParserTreeNode *pParentNode) { m_pParentNode = pParentNode; } 

    // autocompletion methods
    bool AutoComplete(CString &csCommand, AUTOCOMPLETION_STATE *pACState, const bool direction) const;
    static AUTOCOMPLETION_STATE *AllocateNewAutocompletionState();
    static void ResetAutocompletionState (AUTOCOMPLETION_STATE *pACState);

    int GetAvailableArgumentsForCommand(const char *pCommand, vector<CString> &argsOut) const;
    bool Parse(const char *pCommand, CString &statusOut) const;
    const ParserTreeNode *GetParentNode() const { return m_pParentNode; }  // will only be null for root node
    void AppendChildNodeNames(CString &csOut) const;
    void BuildCommandHelpTree(int recursionLevel, CString &csOut);  // cosmetic help string
    
protected:
    // This is the leaf node callback for this node; is null for non-leaf nodes.
    // Note: we do not free these objects; they are managed by whoever passed them to our constructor.
    LeafHandler * const m_pLeafHandler;  // make the pointer itself const so it can't be altered accidentally after it is initialized
    vector<ParserTreeNode *> m_children;

    // These fields maintain state between successive autocompletion calls; this structure is passed to us
    // by the caller.
    struct AutocompletionState 
    {
        int significantCharacters;  // <= 0 means we were just reset, so "test all characters in token"
        int tokenCandidateIndex;    // tracks the index of the last candidate token shown
    };
    
    // member methods
    ParserTreeNode *FindChildForToken(const CString &csToken, AUTOCOMPLETION_STATE *pACState, const bool direction) const;
    const char *AutocompleteToken(const CString &csToken, AUTOCOMPLETION_STATE *pACState, const bool direction, const char **pValidTokenValues) const; 
    int AutoComplete(vector<CString> &argv, const int startingIndex, AUTOCOMPLETION_STATE *pACState, const bool direction) const;  // recursive method
    bool Parse(vector<CString> &argv, const int startingIndex, CString &statusOut) const;  // recursive method
    int GetAvailableArgumentsForCommand(vector<CString> &argv, const int startingIndex, vector<CString> &argsOut) const;  // recursive method
    
    // static utility methods
    static int ParseToSpaceDelimitedTokens(const char *pCommand, vector<CString> &argv);

private:
    const CString *m_pCSNodeText;  // "Set", "MainLeft", etc.  Will be null only for the root node.
    const NodeData *m_pNodeData;   // may be null
    ParserTreeNode *m_pParentNode;
    const int m_nodeGroup;   // arbitrary group ID that groups like nodes together when constructing help strings
};

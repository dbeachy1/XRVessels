//-------------------------------------------------------------------------
// ParserTreeNode.cpp : implementation of ParserTreeNode class; maintains state
// for a given node in our parser tree.
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

#include <windows.h>
#include <limits>
#include "ParserTreeNode.h"

// so numeric_limits<T> min, max will compile
#undef min
#undef max

/*
    Here is an example of how a simple parser tree might look:

                              ParserTreeNode(nullptr)    // root node
                                /              \
                             (Set)              \
                              /               (Config)   
                          (Engine)            /      \
                            /                /        \
    (MainBoth, MainLeft, MainRight,   (AttitudeHold) (AirspeedHold)
     Retro...,Hover..., Scram...)          /             \
                        /                 /               \
                       /                 /                 \
                      /           (Pitch, AOA)         (#targetAirspeed -- this is a leaf node)
                     /                 /
                    /                 /
              (ThrottleLevel,  (#targetX #targetBank -- leaf node)
               GimbalX,
               GimbayY,
               ...)
               /
              /
          (#doubleValue) or (#boolValue)
*/

// Constructor
// csNodeText = "Set", "MainLeft", etc.  If null, denotes the root node of the tree.  Will be cloned internally.
// nodeGroup = arbitrary group ID that groups like nodes together when constructing help strings
// pNodeData = arbitrary data assigned to this node that is for use by the caller as he sees fit.  May be null, although this is normally only null for top-level nodes.
//            Typically, however, this will be data that will be used later by the LeafHandler of this node or one of its children.  This is clone internally.
// pCallback = handler that executes for leaf nodes; should be null for non-leaf nodes.  This is not cloned internally.
ParserTreeNode::ParserTreeNode(const char *pNodeText, const int nodeGroup, const NodeData *pNodeData, LeafHandler *pCallback) :
    m_nodeGroup(nodeGroup), m_pLeafHandler(pCallback), m_pParentNode(nullptr)
{
    m_pCSNodeText = ((pNodeText != nullptr) ? new CString(pNodeText) : NULL);   // clone it
    m_pNodeData = ((pNodeData != nullptr) ? pNodeData->Clone() : NULL);         // deep-clone it
}

// Destructor
ParserTreeNode::~ParserTreeNode()
{
    // recursively free all our child nodes
    for (unsigned int i=0; i < m_children.size(); i++)
        delete m_children[i];

    // free our node text and NodeDAta that we created via cloning in the constructor
    delete m_pCSNodeText;
    delete m_pNodeData;
}

// Add a child node to this node
void ParserTreeNode::AddChild(ParserTreeNode *pChildNode)
{
    _ASSERTE(pChildNode != nullptr); 
    
    pChildNode->SetParentNode(this);   // we are the parent
    m_children.push_back(pChildNode);
}

 
// NOTE: for parsing purposes, all string comparisons are case-insensitive.

// Parse the command and set csCommand to a full auto-completed string if possible.
// Some examples:
//   S -> returns "Set"
//   s m -> returns "Set MainBoth"
//       -> (again) "Set MainLeft"
//
// autocompletionTokenIndex = maintains state as we scroll through possible autocompletion choices
// direction: true = tab direction forward, false = tab direction backward
// Returns true if we autocompleted all commands in csCommand
bool ParserTreeNode::AutoComplete(CString &csCommand, AUTOCOMPLETION_STATE *pACState, const bool direction) const
{
    csCommand = csCommand.Trim();
    if (csCommand.IsEmpty())
        return false;   // nothing to complete
    
    // parse the command into space-separated pieces
    vector<CString> argv;
    ParseToSpaceDelimitedTokens(csCommand, argv);

    // recursively parse all arguments
    const int autocompletedTokenCount = AutoComplete(argv, 0, pACState, direction);

    // now reconstruct the full string from the auto-completed pieces
    csCommand.Empty();
    for (unsigned int i=0; i < argv.size(); i++)
    {
        if (i > 0)
            csCommand += " ";
        csCommand += argv[i];
    }

    const bool autoCompletedAll = (autocompletedTokenCount == argv.size());

    // if we autocompleted all tokens successfully, append a trailing space
    if (autoCompletedAll)
        csCommand += " ";

    return autoCompletedAll;
}

// Recursive method to auto-complete all commands in argv.
// argv = arguments to be autocompleted
// startingIndex = 0-based index at which to start parsing
// autocompletionTokenIndex = maintains state as we scroll through possible autocompletion choices
// direction: true = tab direction forward, false = tab direction backward
// Returns # of nodes auto-completed (may be zero)
int ParserTreeNode::AutoComplete(vector<CString> &argv, const int startingIndex, AUTOCOMPLETION_STATE *pACState, const bool direction) const
{
    _ASSERTE(startingIndex >= 0);
    _ASSERTE(startingIndex < static_cast<int>(argv.size()));
    _ASSERTE(pACState != nullptr);

    int autocompletedTokens = 0;

    // try to parse the requested token by finding a match with one of our child nodes
    CString &csToken = argv[startingIndex];
    
    // By design, only track autocompletion state for the *last* token on the line; otherwise we would 
    // overwrite the command following the one we would autocomplete.
    AUTOCOMPLETION_STATE *pActiveACState = ((startingIndex == (argv.size()-1)) ? pACState : NULL); 
    const int nextArgIndex = startingIndex + 1;
    ParserTreeNode *pMatchingChild = FindChildForToken(csToken, pActiveACState, direction);
    if (pMatchingChild != nullptr)
    {
        // Note: by design, we count a token as autocompleted if even if was already complete 
        autocompletedTokens++;
        argv[startingIndex] = *pMatchingChild->GetNodeText();  // change argv entry to completed token; e.g., "Set", "Main", etc.

        if (nextArgIndex < static_cast<int>(argv.size()))  // any more arguments to parse?
        {
            // now let's recurse down to the next level and try to autocomplete the next level down
            autocompletedTokens += pMatchingChild->AutoComplete(argv, nextArgIndex, pACState, direction);  // propagate the ACState that was passed in
        }
    }
    else  // no matching child
    {
        // let's see if we're a leaf node AND this is the last token on the line (i.e., the first leaf node parameter)
        if ((m_pLeafHandler != nullptr) && (nextArgIndex == static_cast<int>(argv.size())))
        {
            // this is leaf node parameter #1, so let's see if there are any autocompletion tokens available for it
            const char **pFirstParamTokens = m_pLeafHandler->GetFirstParamAutocompletionTokens(this);  // may be NULL
            
            // let's try to find a unique match
            const char *pAutocompletedToken = AutocompleteToken(argv[startingIndex], pACState, direction, pFirstParamTokens);
            if (pAutocompletedToken != nullptr)
            {
                autocompletedTokens++;
                argv[startingIndex] = pAutocompletedToken;  // change argv entry to completed token
                // since this is the last token on the line, there is nothing else to parse: fall through and return
            }
        }
    }

    return autocompletedTokens;
}

// Parse the command until either the entire command is parsed (and executed via its leaf handler) 
// or we locate a syntax or value error.
//
// Returns true on success, false on error
// pCommand = command to be parsed
// statusOut = output buffer for result text
bool ParserTreeNode::Parse(const char *pCommand, CString &statusOut) const
{
    CString csCommand = CString(pCommand).Trim();
    if (csCommand.IsEmpty())
    {
        statusOut = "command is empty.";
        return false;
    }
    
    // parse the command into space-separated pieces
    vector<CString> argv;
    ParseToSpaceDelimitedTokens(csCommand, argv);

    // recursively parse all arguments and execute the command
    CString commandStatus;
    bool success = Parse(argv, 0, commandStatus);

    statusOut.Format("Command: [%s]\r\n", csCommand);
    statusOut += (success ? "" : "Error: ") + commandStatus;

    return success;
}

// Recursive method that will parse the command and recurse down to our child nodes until we execute the
// command or locate a syntax error.
//
// argv = arguments to be parsed
// startingIndex = 0-based index at which to start parsing; NOTE: may be beyond end of argv if this is a leaf node that takes no arguments
// autocompletionTokenIndex = maintains state as we scroll through possible autocompletion choices
// Returns true on success, false on error
bool ParserTreeNode::Parse(vector<CString> &argv, const int startingIndex, CString &statusOut) const
{
    _ASSERTE(startingIndex >= 0);
    // do not validate argv against startingIndex here: may be beyond end of argv if this is a leaf node that takes no arguments

    statusOut.Empty();
    bool retVal = false;   // assume failure

    // if this is a leaf node, invoke the leafHandler execute the action for this node
    if (m_pLeafHandler != nullptr)
    {
        _ASSERTE(m_children.size() == 0);  // leaf nodes must not have any children
        // build vector of remaining arguments
        vector<CString> remainingArgv;
        for (int i=startingIndex; i < static_cast<int>(argv.size()); i++)
            remainingArgv.push_back(argv[i]);
        
        // invoke the leaf handler to execute this command
        retVal = m_pLeafHandler->Execute(this, remainingArgv, statusOut);
    }
    else  // not a leaf node, so let's keep recursing down...
    {
        const int nextArgIndex = startingIndex + 1;
        if (startingIndex < static_cast<int>(argv.size()))  // more arguments to parse?
        {
            // try to parse the requested token by finding a match with one of our child nodes
            CString &csToken = argv[startingIndex];
            ParserTreeNode *pMatchingChild = FindChildForToken(csToken, NULL, true);  // must have exact match here (direction is moot)
            if (pMatchingChild != nullptr)
            {
                // command token is valid
                const int nextArgIndex = startingIndex + 1;
                // Note: there may not be any more arguments to parse here; e.g., for leaf nodes that take no arguments.
                // Therefore, we always recurse down to the next level and attempt to parse/execute it.
                retVal = pMatchingChild->Parse(argv, nextArgIndex, statusOut);
            }
            else   // unknown command
            {
                statusOut.Format("Invalid command token: [%s]", csToken);
                // fall through and return false
            }
        }
        else  // no more arguments, but this is not a leaf node
        {
            statusOut = "Required token missing; options are: ";
            AppendChildNodeNames(statusOut);
            // fall through and return false
        }
    }
    return retVal;
}

// Sets argsOut to a list of bracket-grouped available arguments for the supplied command.
// Returns the level for which the available arguments pertain.  
// For example:
//    "" -> (returns 1), argsOut = [Set, Config, ...]
//    Set -> (returns 2), argsOut = [MainBoth, MainLeft, ...]
//    Set foo -> (returns 2), argsOut = [MainBoth, MainLeft, ...]  (foo is invalid, but the user can still correct 'foo' to be one of the valid options)
//    Set MainBoth -> (returns 3), argsOut = [ThrottleLevel, GimbalX, GimbalY, ...]
//    "foo" -> (returns 1), argsOut = [Set, Config, ...]  (foo is an invalid command)
int ParserTreeNode::GetAvailableArgumentsForCommand(const char *pCommand, vector<CString> &argsOut) const
{
    CString csCommand = CString(pCommand).Trim();

    // parse the command into space-separated pieces
    vector<CString> argv;
    ParseToSpaceDelimitedTokens(csCommand, argv);

    // recursively parse all arguments
    argsOut.empty();
    int argLevel = GetAvailableArgumentsForCommand(argv, 0, argsOut);

    return argLevel;
}

// Recursively parse the supplied command and populate argsOut with bracket-grouped, valid arguments for this command.
// argv = arguments to parse
// startingIndex = index into argv to parse; also denotes our recursion level (0...n)
// argsOut = will be populated with valid arguments for this command
// Returns the level for which the arguments in argsOut pertain.
int ParserTreeNode::GetAvailableArgumentsForCommand(vector<CString> &argv, const int startingIndex, vector<CString> &argsOut) const
{
    _ASSERTE(startingIndex >= 0);
    int retVal;

    // if this is a leaf node, we have reached the end of the chain, so show the leaf handler's help text
    if (m_pLeafHandler != nullptr)
    {
        _ASSERTE(m_children.size() == 0);  // leaf nodes must not have any children
        CString csHelp;
        m_pLeafHandler->GetArgumentHelp(this, csHelp);
        argsOut.push_back("[" + csHelp + "]");  // e.g., "[<double> (range -1.0 - 1.0)]"
        retVal = startingIndex;  // startingIndex also matches our recursion level
    }
    else  // not a leaf node, so let's keep recursing down...
    {
        ParserTreeNode *pMatchingChild = nullptr;
        if (startingIndex < static_cast<int>(argv.size()))  // more arguments to parse?
        {
            // try to parse the requested token by finding a match with one of our child nodes
            CString &csToken = argv[startingIndex];
            pMatchingChild = FindChildForToken(csToken, NULL, true);  // must have exact match here (direction is moot)
        }

        const int nextArgIndex = startingIndex + 1;
        if (pMatchingChild != nullptr)
        {
            // command token is valid, so let's recurse down to the next level (keep parsing)
            const int nextArgIndex = startingIndex + 1;
            retVal = pMatchingChild->GetAvailableArgumentsForCommand(argv, nextArgIndex, argsOut);  // recurse down
        }
        else  
        {
            // No child node found and this is NOT a leaf node, so we have invalid tokens at this level.  
            // Therefore, we return a list of this level's child nodes (available options), grouped in brackets [ ... ].
            int currentNodeGroup;
            for (unsigned int i=0; i < m_children.size(); i++)
            {
                const ParserTreeNode *pChild = m_children[i];
                _ASSERTE(pChild != nullptr);
                CString nodeText = *m_children[i]->GetNodeText();
                
                // enclose a given group of commands in brackets for clarity
                if (i == 0)
                {
                    currentNodeGroup = pChild->GetNodeGroup();  // first node at this level, so its group is the active group now
                    nodeText = " [" + nodeText;  // first group start
                }
                else if (pChild->GetNodeGroup() != currentNodeGroup)
                {
                    // new group coming, so append closing "]" to previous command text and prepend " [" to this command text
                    currentNodeGroup = pChild->GetNodeGroup();  // this is the new active group
                    argsOut.back() += "]";
                    nodeText = " [" + nodeText;  
                }
                argsOut.push_back(nodeText);
            }
            argsOut.back() += "]";  // last group end
            retVal = startingIndex;
        }
   } 
    _ASSERTE(argsOut.size() > 0);
    return retVal;
}

// appends csOut with formatted names for all our child nodes
void ParserTreeNode::AppendChildNodeNames(CString &csOut) const
{
    for (unsigned int i=0; i < m_children.size(); i++)
    {
        if (i > 0)
            csOut += ", ";
        csOut += *m_children[i]->GetNodeText();  // e.g., "Set", "MainBoth", etc.
    }
}

// static factory method that creates a new autocompletion state; it is the caller's responsibility to eventually free this
ParserTreeNode::AUTOCOMPLETION_STATE *ParserTreeNode::AllocateNewAutocompletionState()
{
    AUTOCOMPLETION_STATE *pNew = reinterpret_cast<AUTOCOMPLETION_STATE *>(new ParserTreeNode::AutocompletionState()); 
    ResetAutocompletionState(pNew);
    return pNew;
}

// static utility method to reset the autcompletion state to a new command
void ParserTreeNode::ResetAutocompletionState(AUTOCOMPLETION_STATE *pACState)
{
    AutocompletionState *ptr = reinterpret_cast<AutocompletionState *>(pACState);  // cast back to actual type
    ptr->significantCharacters = 0;
    ptr->tokenCandidateIndex = 0;
}

// Examine our child nodes and try to locate a case-insensitive match for the supplied token.
// acState : tracks autocompletion state between successive autocompletion calls; if null, do not track autocompletion for this token (i.e., this is not the final token on the command line)
// direction: true = tab direction forward, false = tab direction backward
// Returns node on a match or NULL if no match found OR if more than one match found.
ParserTreeNode *ParserTreeNode::FindChildForToken(const CString &csToken, AUTOCOMPLETION_STATE *pACState, const bool direction) const
{
    if (csToken.IsEmpty())
        return nullptr;    // sanity check

    // NOTE: do not modify this object's state *except* for the last token on the command line
    AutocompletionState *pActiveACState = reinterpret_cast<AutocompletionState *>(pACState);  // cast back to actual type

    // assume no autocompletionstate
    int significantCharacters = csToken.GetLength();     
    int tokenCandidateIndex = 0;

    if (pActiveACState != nullptr)
    {
        // we may be stepping through the possible candidates
        significantCharacters = pActiveACState->significantCharacters;
        if (significantCharacters <= 0) 
        {
            // we were reset, so test all characters in the token
            significantCharacters = csToken.GetLength();
        }
        tokenCandidateIndex = pActiveACState->tokenCandidateIndex; 
    }
    
    _ASSERTE(significantCharacters <= csToken.GetLength());

    // step through each of our child nodes and build a list of all case-insensitive matches
    vector<ParserTreeNode *> matchingNodes;
    for (unsigned int i=0; i < m_children.size(); i++)  
    {
        ParserTreeNode *pCandidate = m_children[i];
        const CString csNodeTextPrefix = pCandidate->GetNodeText()->Left(significantCharacters);
        const CString csTokenPrefix = csToken.Left(significantCharacters);
        if (csTokenPrefix.CompareNoCase(csNodeTextPrefix) == 0)
            matchingNodes.push_back(pCandidate);   // we have a match
    }

    // decide which matching node to use
    ParserTreeNode *pRetVal = nullptr;
    const int matchingNodeCount = matchingNodes.size();
    
    if (matchingNodeCount > 0)
    {
        _ASSERTE(tokenCandidateIndex >= 0);
        _ASSERTE(tokenCandidateIndex < matchingNodeCount);

        if (pActiveACState == nullptr)   // not stepping through multiple tokens?
        {
            // must have exactly *one* match or we cannot autocomplete this token
            pRetVal = ((matchingNodeCount == 1) ? matchingNodes.front() : NULL);  
        }
        else   // we're stepping through multiple tokens (always on the last token on the line)
        {
            pRetVal = matchingNodes[tokenCandidateIndex];
            
            // update our AutocompletionState for next time
            pActiveACState->significantCharacters = significantCharacters;
            if (direction)   // forward?
            {
                if (++pActiveACState->tokenCandidateIndex >= matchingNodeCount)
                    pActiveACState->tokenCandidateIndex = 0;  // wrap around to beginning
            }
            else // backward
            {
                if (--pActiveACState->tokenCandidateIndex < 0)
                    pActiveACState->tokenCandidateIndex = (matchingNodeCount - 1);  // wrap around to end
            }
        }
    }
    return pRetVal;
}

// Try to autocomplete the supplied token using the supplied list of valid token values.
// (This method is similar to 'FindChildForToken' above.)
//
// acState : tracks autocompletion state between successive autocompletion calls; if null, do not track autocompletion for this token (i.e., this is not the final token on the command line)
// direction: true = tab direction forward, false = tab direction backward
// pValidTokenValues: may be NULL.  Otherwise, points to a NULL-terminated array of valid token values.
// Returns autocompleted token on a match or NULL if pValidTokenValues is NULL OR no match found OR if more than one match found.
const char *ParserTreeNode::AutocompleteToken(const CString &csToken, AUTOCOMPLETION_STATE *pACState, const bool direction, const char **pValidTokenValues) const
{
    if (pValidTokenValues == nullptr)
        return nullptr;        // no autocompletion possible

    if (csToken.IsEmpty())
        return nullptr;    // sanity check

    // NOTE: do not modify this object's state *except* for the last token on the command line
    AutocompletionState *pActiveACState = reinterpret_cast<AutocompletionState *>(pACState);  // cast back to actual type

    // assume no autocompletionstate
    int significantCharacters = csToken.GetLength();     
    int tokenCandidateIndex = 0;

    if (pActiveACState != nullptr)
    {
        // we may be stepping through the possible candidates
        significantCharacters = pActiveACState->significantCharacters;
        if (significantCharacters <= 0) 
        {
            // we were reset, so test all characters in the token
            significantCharacters = csToken.GetLength();
        }
        tokenCandidateIndex = pActiveACState->tokenCandidateIndex; 
    }
    
    _ASSERTE(significantCharacters <= csToken.GetLength());

    // step through each of our valid tokens and build a list of all case-insensitive matches
    vector<const char *> matchingTokens;   
    for (const char **ppValidToken = pValidTokenValues; *ppValidToken != nullptr; ppValidToken++)
    {
        CString candidate = *ppValidToken;    // valid token (a possible match)
        const CString csNodeTextPrefix = candidate.Left(significantCharacters);
        const CString csTokenPrefix = csToken.Left(significantCharacters);
        if (csTokenPrefix.CompareNoCase(csNodeTextPrefix) == 0)
            matchingTokens.push_back(*ppValidToken);   // we have a match
    }

    // decide which matching node to use
    const char *pRetVal = nullptr;
    const int matchingTokenCount = matchingTokens.size();

    if (matchingTokenCount > 0)
    {
        _ASSERTE(tokenCandidateIndex >= 0);
        _ASSERTE(tokenCandidateIndex < matchingTokenCount);

        if (pActiveACState == nullptr)   // not stepping through multiple tokens?
        {
            // must have exactly *one* match or we cannot autocomplete this token
            pRetVal = ((matchingTokenCount == 1) ? matchingTokens.front() : NULL);  
        }
        else   // we're stepping through multiple tokens (always on the last token on the line)
        {
            pRetVal = matchingTokens[tokenCandidateIndex];
            
            // update our AutocompletionState for next time
            pActiveACState->significantCharacters = significantCharacters;
            if (direction)   // forward?
            {
                if (++pActiveACState->tokenCandidateIndex >= matchingTokenCount)
                    pActiveACState->tokenCandidateIndex = 0;  // wrap around to beginning
            }
            else // backward
            {
                if (--pActiveACState->tokenCandidateIndex < 0)
                    pActiveACState->tokenCandidateIndex = (matchingTokenCount - 1);  // wrap around to end
            }
        }
    }
    return pRetVal;
}

// static utility method that will parse a given command string into space-delimited tokens
// argv = contains parse-delmited tokens; NOTE: it is the caller's responsibility to free the CString objects
//        added to the vector.
// Returns: # of valid (i.e., non-empty) tokens parsed; i.e., argv.size()
int ParserTreeNode::ParseToSpaceDelimitedTokens(const char *pCommand, vector<CString> &argv)
{
    CString csCommand(pCommand);
    int tokenIndex = 0;
    while (tokenIndex >= 0)
    {
        CString token = csCommand.Tokenize(" ", tokenIndex);
        if (!token.IsEmpty())
        {
            argv.push_back(token.Trim());  // whack any other non-printables
        }
    }

    return argv.size();
}

//
// LeafHandler static utility methods
//

// Parse a validated double from the supplied string.
//
// Parameters:
//   pStr = string to be parsed
//   dblOut = will be set to parsed value, regardless of whether it is in range
//   min = minimum valid value, inclusive
//   max = maximum valid value, inclusive
//   pCSErrorMsgOut = if non-null, will be set to error reason if parse or validation fails
//
// Returns: true if value parsed successfully and is in range, false otherwise.
bool ParserTreeNode::LeafHandler::ParseValidatedDouble(const char *pStr, double &dblOut, const double min, const double max, CString *pCSErrorMsgOut)
{
    bool parseSuccessful = ParseDouble(pStr, dblOut);
    bool inRange = ((dblOut >= min) && (dblOut <= max));
    if (pCSErrorMsgOut != nullptr)
    {
        if (!parseSuccessful)
        {
            pCSErrorMsgOut->Format("Invalid argument: '%s'", pStr);
        }
        else  // parse successful
        {
            if (!inRange)
            {
                if ((min != numeric_limits<double>::min()) && (max != numeric_limits<double>::max()))
                {
                    // normal limits defined
                    pCSErrorMsgOut->Format("Value out-of-range (%.4lf); valid range is %.4lf - %.4lf.", dblOut, min, max);
                }
                else if (min == numeric_limits<double>::min())  
                {
                    // upper limit, but no lower limit
                    pCSErrorMsgOut->Format("Value too large (%.4lf); must be <= %.4lf.", dblOut, max);
                }
                else // must be  max == numeric_limits<double>::max()
                {
                    // lower limit, but no upper limit
                    pCSErrorMsgOut->Format("Value too small (%.4lf); must be >= %.4lf.", dblOut, min);
                }
            }   
        }
    }
    return inRange;
}

// Parse a validated boolean from the supplied string.
//
// Parameters:
//   pStr = string to be parsed; for success, must be one of "true", "on", "false", or "off" (case-insensitive)
//   boolOut = will be set to parsed value, regardless of whether it is valid
//   pCSErrorMsgOut = if non-null, will be set to error reason if parse fails
//
// Returns: true if value parsed is valid, false otherwise
bool ParserTreeNode::LeafHandler::ParseValidatedBool(const char *pStr, bool &boolOut, CString *pCSErrorMsgOut)
{
    bool success = ParseBool(pStr, boolOut);
    
    if ((pCSErrorMsgOut != nullptr) && (!success))
        pCSErrorMsgOut->Format("Invalid boolean value (%s); valid options are 'true', 'on', 'false', or 'off' (case-insensitive).", pStr);

    return success;
}

// Parse a validated integer from the supplied string.
//
// Parameters:
//   pStr = string to be parsed
//   intOut = will be set to parsed value, regardless of whether it is in range
//   min = minimum valid value, inclusive
//   max = maximum valid value, inclusive
//   pCSErrorMsgOut = if non-null, will be set to error reason if parse or validation fails
//
// Returns: true if value parsed successfully and is in range, false otherwise.
bool ParserTreeNode::LeafHandler::ParseValidatedInt(const char *pStr, int &intOut, const int min, const int max, CString *pCSErrorMsgOut)
{
    bool parseSuccessful = ParseInt(pStr, intOut);
    bool inRange = (parseSuccessful && (intOut >= min) && (intOut <= max));

    if (pCSErrorMsgOut != nullptr)
    {
        if (!parseSuccessful)
            pCSErrorMsgOut->Format("Invalid argument: '%s'", pStr);
        else if (!inRange)   // value parsed successfully, but is it out-of-range?
            pCSErrorMsgOut->Format("Value out-of-range (%d); valid range is %d - %d.", intOut, min, max);
    }
    return inRange;
}

// Parse a double from the supplied string; returns true on success, false on error.
// On success, dblOut will contain the parsed value.
// Returns true if value parsed successfully, or false if value could not be parsed (invalid string).
bool ParserTreeNode::LeafHandler::ParseDouble(const char *pStr, double &dblOut)
{
    _ASSERTE(pStr != nullptr);

    // we use sscanf_s instead of atof here because it has error handling
    return (sscanf_s(pStr, "%lf", &dblOut) == 1);
}

// Parse a boolean from the supplied string; returns true on success, false on error.
// pStr: should be one of "true", "on", "false", or "off" (case-insensitive).
// On success, boolOut will contain the parsed value.
// Returns true if value parsed successfully, or false if value could not be parsed (invalid string).
bool ParserTreeNode::LeafHandler::ParseBool(const char *pStr, bool &boolOut)
{
    _ASSERTE(pStr != nullptr);

    bool success = false;
    if (!_stricmp(pStr, "true") || !_stricmp(pStr, "on"))
    {
        boolOut = success = true;
    }
    else if (!_stricmp(pStr, "false") || !_stricmp(pStr, "off"))
    {
        boolOut = false;
        success = true;
    }
    // else fall through and return false

    return success;
}

// Parse an integer from the supplied string; returns true on success, false on error.
// On success, intOut will contain the parsed value.
// Returns true if value parsed successfully, or false if value could not be parsed (invalid string)
bool ParserTreeNode::LeafHandler::ParseInt(const char *pStr, int &intOut)
{
    _ASSERTE(pStr != nullptr);

    // we use sscanf_s instead of atof here because it has error handling
    return (sscanf_s(pStr, "%d", &intOut) == 1);
}

// Recursively build a tree of all command help text appended to csOut.
// indent = indent for this line in csOut.
void ParserTreeNode::BuildCommandHelpTree(int recursionLevel, CString &csOut)
{
    // build indent string
    CString csIndent;
    for (int i=0; i < recursionLevel; i++)
        csIndent += "    ";

    csOut += csIndent;  // indent this line

    // add our command text
    const CString *pNodeText = GetNodeText();
    if (pNodeText != nullptr)
    {
        csOut += *pNodeText;
        csOut += " ";
    }

    // if we're a leaf node, see if we have any help text
    if (m_pLeafHandler != nullptr)
    {
        CString csLeafHelp;
        m_pLeafHandler->GetArgumentHelp(this, csLeafHelp);
        csOut += csLeafHelp;  // add the leaf node text, too
    }

    // terminate this line 
    if (csOut.GetLength() > 0)  // prevent extra root node newline and indent
    {
        csOut += "\r\n";
        recursionLevel++;
    }
    
    // recurse down to all our children
    for (unsigned i=0; i < m_children.size(); i++)
        m_children[i]->BuildCommandHelpTree(recursionLevel, csOut);

    if (m_children.size() > 0)  // not a leaf node?
        csOut += "\r\n";   // add separator line
}

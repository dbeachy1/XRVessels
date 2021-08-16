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
// XRVCClientCommandParser.h : definition of XRVCClientCommandParser class.
//-------------------------------------------------------------------------

#pragma once

#include <vector>
#include "XRVCClient.h"
#include "ParserTree.h"

// Handles command parsing tasks for XRVCClient
class XRVCClientCommandParser
{
public:
    XRVCClientCommandParser(XRVCClient &xrvcClient);
    virtual ~XRVCClientCommandParser();

    bool ExecuteCommand(const CString &command, CString &statusOut);  // runs a command and stores status to statusOut

    bool AutoCompleteCommand(CString &csCommand, const bool direction) const { return m_commandParserTree->AutoComplete(csCommand, direction); }  // returns true if we autocompleted all tokens in csCommand
    int GetAvailableArgumentsForCommand(CString &csCommand, vector<CString> &argsOut) const { return m_commandParserTree->GetAvailableArgumentsForCommand(csCommand, argsOut); }
    const char *RetrieveCommand(const bool getNext);  // returns next/previous executed command
    void ResetCommandRecallIndex() { m_commandRecallIndex = static_cast<int>(m_commandHistoryVector.size()); }  // reset to 1 beyond the end of the vector, which denotes "empty line"
    void ResetAutocompletionState() { m_commandParserTree->ResetAutocompletionState(); }  // invoked when any non-tab character pressed

    // this method is only used for debugging
    void BuildCommandHelpTree(CString &csOut) { m_commandParserTree->BuildCommandHelpTree(csOut); }

protected:
    XRVCClient &m_xrvcClient;         // performs all the XRVesselCtrl calls
    ParserTree *m_commandParserTree;  // root node of the parser tree
    vector<const CString *> m_commandHistoryVector;  // order is oldest -> newest
    int m_commandRecallIndex;         // index into m_commandHistoryVector of last command recalled; -1 = no recall yet


    // other static utility methods
    static bool ValidateArgumentCount(const int argc, const int minArgs, const int maxArgs, CString &statusOut);

    // Base class common to all our NodeData subclasses here, or may be 
    // use by itself if additional state data is not necessary.
    struct BaseNodeData : public ParserTreeNode::NodeData
    {
        BaseNodeData(XRVCClient &client) : xrvcClient(client) { }
        XRVCClient &xrvcClient;  // owning object instance

        // this method should be overridden by subclasses
        virtual NodeData *Clone() const { return new BaseNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    // define parser callback data objects; each is passed to a different type of leaf node
    struct EngineNodeData : public BaseNodeData
    {
        EngineNodeData(XRVCClient &client) : BaseNodeData(client) { }
        XREngineID engine1;
        XREngineID engine2;  // will match engine1 if we're only driving one engine
        XRVCClient::DataType dataType;   // Double or Bool
        void *pValueToSet;   // pointer to value in XREngineStateWrite that will be set
        double minDblValue;  // ignored for boolean values
        double maxDblValue;

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new EngineNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    struct DoorNodeData : public BaseNodeData
    {
        DoorNodeData(XRVCClient &client, XRDoorID doorID) : BaseNodeData(client), doorID(doorID) { }
        XRDoorID doorID;

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new DoorNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    struct EnumBoolNodeData : public BaseNodeData
    {
        EnumBoolNodeData(XRVCClient &client) : BaseNodeData(client), enumID(-1) { }
        int enumID; 
        bool (XRVCClient::*method)(const int id, const bool on, CString &csOut) const;  // callback method that performs the XR command

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new EnumBoolNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    struct SingleIntNodeData : public BaseNodeData
    {
        SingleIntNodeData(XRVCClient &client) : BaseNodeData(client) { }
        int limitLow;
        int limitHigh;
        bool (XRVCClient::*method)(const int value, CString &csOut) const;  // callback method that performs the XR command
        bool IsBoolArgument() const  { return ((limitLow == 0) && (limitHigh == 1)); }

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new SingleIntNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    struct SingleDoubleNodeData : public BaseNodeData
    {
        SingleDoubleNodeData(XRVCClient &client) : BaseNodeData(client) { }
        double limitLow;
        double limitHigh;
        bool (XRVCClient::*method)(const double value, CString &csOut) const;  // callback method that performs the XR command

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new SingleDoubleNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    struct StdAutopilotNodeData : public BaseNodeData
    {
        StdAutopilotNodeData(XRVCClient &client) : BaseNodeData(client) { }
        XRStdAutopilot autopilotID;

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new StdAutopilotNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    struct DamageStateNodeData : public BaseNodeData
    {
        DamageStateNodeData(XRVCClient &client) : BaseNodeData(client) { }
        XRVCClient::DataType dataType;   // Double (0 - 1.0) or Int (XRDamageState)
        void *pValueToSet;   // pointer to value in XRSystemStatusWrite that will be set

        // implement the NodeData interface
        virtual NodeData *Clone() const { return new DamageStateNodeData(*this); }  // default byte-for-byte copy constructor is sufficient
    };

    //
    // Define parser callback objects; each extends ParserTreeNode::LeafHandler
    //
    struct EngineLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const;
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode); 
    };

    struct DoorLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const  { csOut = "opening  open  closing  closed "; }  
        // Note: 'open' should be listed first so it will not be autocompleted to 'opening'
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { static const char *s_pTokens[] =  { "open", "opening", "closing", "closed", nullptr }; return s_pTokens; }  
        static XRDoorState ParseDoorState(const char *pArg);
    };
    
    struct EnumBoolLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const  { csOut = "on/true  off/false"; }  
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { static const char *s_pTokens[] =  { "on", "off", nullptr }; return s_pTokens; } 
    };

    struct SingleIntLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const;
    };

    struct SingleDoubleLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const;
    };

    struct AttitudeHoldLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const { csOut = "on/off  [Pitch/AOA  <double>TargetPitch  <double>TargetBank]"; }
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { static const char *s_pTokens[] =  { "on", "off", nullptr }; return s_pTokens; } 
    };

    struct DescentHoldLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const { csOut = "on/off  [<double>TargetDescentRate]  [<bool>AutoLandMode]"; }
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { static const char *s_pTokens[] =  { "on", "off", nullptr }; return s_pTokens; } 
    };

    struct AirspeedHoldLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const { csOut = "on/off  [<double>TargetAirspeed]"; }
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { static const char *s_pTokens[] =  { "on", "off", nullptr }; return s_pTokens; } 
    };

    struct SimpleResetLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const { csOut = "Autopilots | MasterWarning | Damage"; }
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode) { static const char *s_pTokens[] =  { "Autopilots", "MasterWarning", "Damage", nullptr }; return s_pTokens; } 
    };

    struct DamageStateLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const;
        virtual const char **GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode); 
    };

    struct RunScriptLeafHandler : public ParserTreeNode::LeafHandler
    {
        virtual bool Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut);
        virtual void GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const { csOut = "<filepath\\filename>"; }
    };

    // member data containing parser leaf callback pointers
    EngineLeafHandler *m_pEngineLeafHandler;  // writes a value to XREngineStateWrite
    DoorLeafHandler *m_pDoorLeafHandler;
    EnumBoolLeafHandler *m_pEnumBoolLeafHandler;
    SingleIntLeafHandler *m_pSingleIntLeafHandler;
    SingleDoubleLeafHandler *m_pSingleDoubleLeafHandler;
    AttitudeHoldLeafHandler *m_pAttitudeHoldLeafHandler;
    DescentHoldLeafHandler *m_pDescentHoldLeafHandler;
    AirspeedHoldLeafHandler *m_pAirspeedHoldLeafHandler;
    SimpleResetLeafHandler *m_pSimpleResetLeafHandler;
    DamageStateLeafHandler *m_pDamageStateLeafHandler;
    RunScriptLeafHandler *m_pRunScriptLeafHandler;

private:
    void InitializeCommandParserTree();   // invoked from constructor
    void FreeCommandParserTree();         // invoked from destructor
};

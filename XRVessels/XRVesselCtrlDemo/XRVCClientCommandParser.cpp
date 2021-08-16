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
// XRVCClientCommandParser.cpp : implementation of XRVCClientCommandParser class.
//-------------------------------------------------------------------------

#include <windows.h>
#include <limits>
#include <io.h>     // for _access_s
#include "XRVCClientCommandParser.h"
#include "XRVCMainDialog.h"  // for ExecutScript callback

// these two lines are necessary in order for numeric_limits::min and max to compile
#undef min
#undef max

// Constructor
XRVCClientCommandParser::XRVCClientCommandParser(XRVCClient &xrvcClient) : 
    m_xrvcClient(xrvcClient), m_commandRecallIndex(-1)
{ 
    InitializeCommandParserTree();   
}

// Destructor
XRVCClientCommandParser::~XRVCClientCommandParser()
{
    // recursively free the entire parser tree and our tree callback objects
    FreeCommandParserTree();

    // free all our our stored history commands
    for (unsigned int i=0; i < m_commandHistoryVector.size(); i++)
        delete m_commandHistoryVector[i];

}

//-------------------------------------------------------------------------
// invoked from destructor
//-------------------------------------------------------------------------
void XRVCClientCommandParser::FreeCommandParserTree()
{
    // recursively free all our tree nodes 
    delete m_commandParserTree;

    // free our leaf handler callback objects
    delete m_pEngineLeafHandler;
    delete m_pDoorLeafHandler;
    delete m_pEnumBoolLeafHandler;
    delete m_pSingleIntLeafHandler;
    delete m_pSingleDoubleLeafHandler;
    delete m_pAttitudeHoldLeafHandler;
    delete m_pDescentHoldLeafHandler;
    delete m_pAirspeedHoldLeafHandler;
    delete m_pSimpleResetLeafHandler;
    delete m_pDamageStateLeafHandler;
    delete m_pRunScriptLeafHandler;
}

//-------------------------------------------------------------------------
// invoked from constructor
//-------------------------------------------------------------------------
void XRVCClientCommandParser::InitializeCommandParserTree()
{
    // Initialize our parser tree callback objects
    m_pEngineLeafHandler = new EngineLeafHandler();
    m_pDoorLeafHandler = new DoorLeafHandler();
    m_pEnumBoolLeafHandler = new EnumBoolLeafHandler();
    m_pSingleIntLeafHandler = new SingleIntLeafHandler();
    m_pSingleDoubleLeafHandler = new SingleDoubleLeafHandler();
    m_pAttitudeHoldLeafHandler = new AttitudeHoldLeafHandler();
    m_pDescentHoldLeafHandler = new DescentHoldLeafHandler();
    m_pAirspeedHoldLeafHandler = new AirspeedHoldLeafHandler();
    m_pSimpleResetLeafHandler = new SimpleResetLeafHandler();
    m_pDamageStateLeafHandler = new DamageStateLeafHandler();
    m_pRunScriptLeafHandler = new RunScriptLeafHandler();

    //
    // Build our parser tree
    //
    m_commandParserTree = new ParserTree();

    //
    // Level-1 commands
    //
    BaseNodeData baseNodeData(m_xrvcClient);
    int nodeGroup = 0;      // incremented as we add each successive group (keeps like commands grouped together)
    ParserTreeNode* pptnSet = new ParserTreeNode("Set", nodeGroup);
    m_commandParserTree->AddTopLevelNode(pptnSet);

    // Reset [Autopilots | MWS | Damage]  (this is a top-level leaf node)
    nodeGroup++;
    ParserTreeNode* pptnReset = new ParserTreeNode("Reset", nodeGroup, &baseNodeData, m_pSimpleResetLeafHandler);
    m_commandParserTree->AddTopLevelNode(pptnReset);

    // Runscript 
    nodeGroup++;
    m_commandParserTree->AddTopLevelNode(new ParserTreeNode("Runscript", nodeGroup, &baseNodeData, m_pRunScriptLeafHandler));

    // Shift center-of-gravity
    nodeGroup++;
    SingleDoubleNodeData singleDoubleNodeData(m_xrvcClient);
    // limits are vessel-specific, so do not limit them here
    singleDoubleNodeData.limitLow = -numeric_limits<double>::max();
    singleDoubleNodeData.limitHigh = numeric_limits<double>::max();
    singleDoubleNodeData.method = &XRVCClient::ShiftCenterOfGravity;
    m_commandParserTree->AddTopLevelNode(new ParserTreeNode("ShiftCenterOfGravity", nodeGroup, &singleDoubleNodeData, m_pSingleDoubleLeafHandler));

    //
    // Level-2 commands
    //
    nodeGroup++;
    ParserTreeNode* pptnEngine       = new ParserTreeNode("Engine", nodeGroup);
    ParserTreeNode* pptnDoor         = new ParserTreeNode("Door", nodeGroup);
    ParserTreeNode* pptnLight        = new ParserTreeNode("Light", nodeGroup);
    ParserTreeNode* pptnStdAutopilot = new ParserTreeNode("StdAutopilot", nodeGroup);
    ParserTreeNode* pptnXRAutopilot  = new ParserTreeNode("XRAutopilot", nodeGroup);
    ParserTreeNode* pptnDamageState  = new ParserTreeNode("DamageState", nodeGroup);
    ParserTreeNode* pptnOther        = new ParserTreeNode("Other", nodeGroup);
    pptnSet->AddChild(pptnEngine);
    pptnSet->AddChild(pptnDoor);
    pptnSet->AddChild(pptnLight);
    pptnSet->AddChild(pptnStdAutopilot);
    pptnSet->AddChild(pptnXRAutopilot);
    pptnSet->AddChild(pptnDamageState);
    pptnSet->AddChild(pptnOther);

    //
    // 'Set Engine' commands
    //
    nodeGroup++;

    // SET Engine commands; each engine node is identical except for the engine IDs (the command)
    struct EngineData { const char *command; XREngineID engineID1; XREngineID engineID2; };

    EngineData engineDataArray[] =
    {
        { "MainBoth",   XREngineID::XRE_MainLeft,  XREngineID::XRE_MainRight },
        { "MainLeft",   XREngineID::XRE_MainLeft,  XREngineID::XRE_MainLeft  },  // only one engine 
        { "MainRight",  XREngineID::XRE_MainRight, XREngineID::XRE_MainRight },

        { "HoverBoth",  XREngineID::XRE_HoverFore, XREngineID::XRE_HoverAft  },
        { "HoverFore",  XREngineID::XRE_HoverFore, XREngineID::XRE_HoverFore },
        { "HoverAft",   XREngineID::XRE_HoverAft,  XREngineID::XRE_HoverAft  },

        { "ScramBoth",  XREngineID::XRE_ScramLeft,  XREngineID::XRE_ScramRight },
        { "ScramLeft",  XREngineID::XRE_ScramLeft,  XREngineID::XRE_ScramLeft  },
        { "ScramRight", XREngineID::XRE_ScramRight, XREngineID::XRE_ScramRight },

        { "RetroBoth",  XREngineID::XRE_RetroLeft,  XREngineID::XRE_RetroRight },
        { "RetroLeft",  XREngineID::XRE_RetroLeft,  XREngineID::XRE_RetroLeft  },
        { "RetroRight", XREngineID::XRE_RetroRight, XREngineID::XRE_RetroRight },
    };

    for (int i = 0; i < (sizeof(engineDataArray) / sizeof(EngineData)); i++)
    {
        const EngineData* pDat = engineDataArray + i;     // points to a row in the array
        EngineNodeData nodeData(m_xrvcClient);  // reused and cloned for each leaf below
        nodeData.engine1 = pDat->engineID1;
        nodeData.engine2 = pDat->engineID2;
        // Note: the remaining fields are set and reused by the ADD_ENGINE_LEAF macro below

        // build the middle node ("MainBoth", "MainLeft", etc.)
        ParserTreeNode* pEngineNode = new ParserTreeNode(pDat->command, nodeGroup);  // e.g., "MainBoth", "MainLeft", etc.
        pptnEngine->AddChild(pEngineNode);  // Set->Engine->MainLeft, Set->Engine->MainRight, etc.

        // Now build the leaf nodes underneath ("ThrottleLevel", "GimbalX", etc.)
        //
        // e.g., nodeData.pValueToSet = &m_xrEngineStateWrite.ThrottleLevel; 
        //       pEngineNode->AddChild(new ParserTreeNode("ThrottleLevel", &nodeData, &s_doubleLeafHandler));
#define ADD_ENGINE_LEAF(FIELD, DATATYPE, MINVALUE, MAXVALUE)                        \
            nodeData.dataType = DATATYPE;                                           \
            nodeData.pValueToSet = &m_xrvcClient.GetXREngineStateWrite().##FIELD;   \
            nodeData.minDblValue = MINVALUE;                                        \
            nodeData.maxDblValue = MAXVALUE;                                        \
            pEngineNode->AddChild(new ParserTreeNode(#FIELD, (nodeGroup+1), &nodeData, m_pEngineLeafHandler))

        // define the leaf nodes for EngineStateWrite    
        ADD_ENGINE_LEAF(ThrottleLevel, XRVCClient::DataType::Double, 0.0, 1.0);
        ADD_ENGINE_LEAF(GimbalX, XRVCClient::DataType::Double, -1.0, 1.0);
        ADD_ENGINE_LEAF(GimbalY, XRVCClient::DataType::Double, -1.0, 1.0);
        ADD_ENGINE_LEAF(Balance, XRVCClient::DataType::Double, -1.0, 1.0);
        ADD_ENGINE_LEAF(CenteringModeX, XRVCClient::DataType::Bool, 0, 0);
        ADD_ENGINE_LEAF(CenteringModeY, XRVCClient::DataType::Bool, 0, 0);
        ADD_ENGINE_LEAF(CenteringModeBalance, XRVCClient::DataType::Bool, 0, 0);
        ADD_ENGINE_LEAF(AutoMode, XRVCClient::DataType::Bool, 0, 0);
        ADD_ENGINE_LEAF(DivergentMode, XRVCClient::DataType::Bool, 0, 0);
    }
    nodeGroup++;  // skip the leaf node group we used above

    // Set Door comands; each door node is identical except for the door ID.
    // 
    // Build the leaf nodes ("DockingPort", "ScramDoors", etc.):
    // e.g., DoorNodeData nodeData(*this, XRD_DockingPort);    
    //       pptnDoor->AddChild(ParserTreeNode("DockingPort"))
    // Note: local DoorNodeData object is deep-cloned inside ParserTreeNode's constructor
    nodeGroup++;
#define ADD_DOOR_LEAF(FIELD) {                                          \
            DoorNodeData nodeData(m_xrvcClient, XRDoorID::XRD_##FIELD);           \
            pptnDoor->AddChild(new ParserTreeNode(#FIELD, nodeGroup, &nodeData, m_pDoorLeafHandler));  } 

    ADD_DOOR_LEAF(DockingPort);
    ADD_DOOR_LEAF(ScramDoors);
    ADD_DOOR_LEAF(HoverDoors);
    ADD_DOOR_LEAF(Ladder);
    ADD_DOOR_LEAF(Gear);
    ADD_DOOR_LEAF(RetroDoors);
    ADD_DOOR_LEAF(OuterAirlock);
    ADD_DOOR_LEAF(InnerAirlock);
    ADD_DOOR_LEAF(AirlockChamber);
    ADD_DOOR_LEAF(CrewHatch);
    ADD_DOOR_LEAF(Radiator);
    ADD_DOOR_LEAF(Speedbrake);
    ADD_DOOR_LEAF(APU);
    ADD_DOOR_LEAF(CrewElevator);
    ADD_DOOR_LEAF(PayloadBayDoors);

    //
    // Set Light state comands; each light node is identical except for the light enum ID.
    //
    nodeGroup++;
    EnumBoolNodeData enumBoolNodeData(m_xrvcClient);
    enumBoolNodeData.method = &XRVCClient::UpdateLightState;
#define ADD_LIGHT_LEAF(FIELD)                                                 \
        enumBoolNodeData.enumID = static_cast<int>(XRLight::XRL_##FIELD); \
        pptnLight->AddChild(new ParserTreeNode(#FIELD, nodeGroup, &enumBoolNodeData, m_pEnumBoolLeafHandler))

    ADD_LIGHT_LEAF(Nav);
    ADD_LIGHT_LEAF(Beacon);
    ADD_LIGHT_LEAF(Strobe);

    //
    // Set Other state commands.
    // Each of these leaf nodes take a single int (or BOOL) as an argument, so we can use the same handler for each
    //
    SingleIntNodeData singleIntNodeData(m_xrvcClient);   // reused below for each single-int command
#define ADD_SINGLEINT_LEAF(COMMAND_NAME, LIMIT_LOW, LIMIT_HIGH, METHOD) \
        nodeGroup++;                                        \
        singleIntNodeData.limitLow = LIMIT_LOW;             \
        singleIntNodeData.limitHigh = LIMIT_HIGH;           \
        singleIntNodeData.method = &XRVCClient::METHOD;     \
        pptnOther->AddChild(new ParserTreeNode(#COMMAND_NAME, nodeGroup, &singleIntNodeData, m_pSingleIntLeafHandler))

    ADD_SINGLEINT_LEAF(SecondaryHUDMode, 0, 5, SetSecondaryHUDMode);
    ADD_SINGLEINT_LEAF(SetTertiaryHUDState, 0, 1, SetTertiaryHUDState);        // BOOL
    ADD_SINGLEINT_LEAF(RCSDockingMode, 0, 1, SetRCSDockingMode);               // BOOL
    ADD_SINGLEINT_LEAF(ElevatorEVAPortActive, 0, 1, SetElevatorEVAPortActive); // BOOL

    //
    // Set DamageState <system> <state>
    //
    nodeGroup++;

    // Now build a leaf node for each damage leaf nodes underneath ("LeftWing", "RightWing", etc.)
    //
    // e.g., nodeData.pValueToSet = &m_xrEngineStateWrite.ThrottleLevel; 
    //       pEngineNode->AddChild(new ParserTreeNode("LeftWing", &nodeData, &s_doubleLeafHandler));
    DamageStateNodeData damageStateNodeData(m_xrvcClient);    // cloned and reused for each leaf

#define ADD_DAMAGE_LEAF_DBL(FIELD)  ADD_DAMAGE_LEAF(FIELD, XRVCClient::DataType::Double)
#define ADD_DAMAGE_LEAF_INT(FIELD)  ADD_DAMAGE_LEAF(FIELD, XRVCClient::DataType::Int)

#define ADD_DAMAGE_LEAF(FIELD, DATATYPE)                                         \
    damageStateNodeData.dataType = DATATYPE;                                 \
    damageStateNodeData.pValueToSet = &m_xrvcClient.GetXRSystemStatusWrite().##FIELD;  \
    pptnDamageState->AddChild(new ParserTreeNode(#FIELD, (nodeGroup+1), &damageStateNodeData, m_pDamageStateLeafHandler))

    // define the leaf nodes for EngineStateWrite    
    ADD_DAMAGE_LEAF_DBL(LeftWing);
    ADD_DAMAGE_LEAF_DBL(RightWing);
    ADD_DAMAGE_LEAF_DBL(LeftMainEngine);
    ADD_DAMAGE_LEAF_DBL(RightMainEngine);
    ADD_DAMAGE_LEAF_DBL(LeftSCRAMEngine);
    ADD_DAMAGE_LEAF_DBL(RightSCRAMEngine);
    ADD_DAMAGE_LEAF_DBL(ForeHoverEngine);
    ADD_DAMAGE_LEAF_DBL(AftHoverEngine);
    ADD_DAMAGE_LEAF_DBL(LeftRetroEngine);
    ADD_DAMAGE_LEAF_DBL(RightRetroEngine);
    ADD_DAMAGE_LEAF_DBL(ForwardLowerRCS);
    ADD_DAMAGE_LEAF_DBL(AftUpperRCS);
    ADD_DAMAGE_LEAF_DBL(ForwardUpperRCS);
    ADD_DAMAGE_LEAF_DBL(AftLowerRCS);
    ADD_DAMAGE_LEAF_DBL(ForwardStarboardRCS);
    ADD_DAMAGE_LEAF_DBL(AftPortRCS);
    ADD_DAMAGE_LEAF_DBL(ForwardPortRCS);
    ADD_DAMAGE_LEAF_DBL(AftStarboardRCS);
    ADD_DAMAGE_LEAF_DBL(OutboardUpperPortRCS);
    ADD_DAMAGE_LEAF_DBL(OutboardLowerStarboardRCS);
    ADD_DAMAGE_LEAF_DBL(OutboardUpperStarboardRCS);
    ADD_DAMAGE_LEAF_DBL(OutboardLowerPortRCS);
    ADD_DAMAGE_LEAF_DBL(AftRCS);
    ADD_DAMAGE_LEAF_DBL(ForwardRCS);

    ADD_DAMAGE_LEAF_INT(LeftAileron);
    ADD_DAMAGE_LEAF_INT(RightAileron);
    ADD_DAMAGE_LEAF_INT(LandingGear);
    ADD_DAMAGE_LEAF_INT(DockingPort);
    ADD_DAMAGE_LEAF_INT(RetroDoors);
    ADD_DAMAGE_LEAF_INT(TopHatch);
    ADD_DAMAGE_LEAF_INT(Radiator);
    ADD_DAMAGE_LEAF_INT(Speedbrake);
    ADD_DAMAGE_LEAF_INT(PayloadBayDoors);
    ADD_DAMAGE_LEAF_INT(CrewElevator);

    nodeGroup++;  // skip the leaf node group we used above

    //
    // Set StdAutopilot <name> [on/off]
    //
    nodeGroup++;
    enumBoolNodeData.method = &XRVCClient::SetStdAutopilotState;   // object updated and reused below (deep-cloned)
#define ADD_STDAUTOPILOT_LEAF(FIELD)                                    \
        enumBoolNodeData.enumID = static_cast<int>(XRStdAutopilot::XRSAP_##FIELD);  \
        pptnStdAutopilot->AddChild(new ParserTreeNode(#FIELD, nodeGroup, &enumBoolNodeData, m_pEnumBoolLeafHandler))

    ADD_STDAUTOPILOT_LEAF(KillRot);
    ADD_STDAUTOPILOT_LEAF(Prograde);
    ADD_STDAUTOPILOT_LEAF(Retrograde);
    ADD_STDAUTOPILOT_LEAF(Normal);
    ADD_STDAUTOPILOT_LEAF(AntiNormal);
    ADD_STDAUTOPILOT_LEAF(LevelHorizon);
    ADD_STDAUTOPILOT_LEAF(Hover);

    //
    // Set XRAutopilot <name> [... params ...]
    //
    nodeGroup++;
    // reuse BaseNodeData we already defined
#define ADD_XRAUTOPILOT_LEAF(NAME)                  \
    pptnXRAutopilot->AddChild(new ParserTreeNode(#NAME, nodeGroup, &baseNodeData, m_p##NAME##LeafHandler))

    ADD_XRAUTOPILOT_LEAF(AttitudeHold);
    ADD_XRAUTOPILOT_LEAF(DescentHold);
    ADD_XRAUTOPILOT_LEAF(AirspeedHold);
}

// Returns next/previous executed command (e.g., from up/down arrow), or empty string if there is no next/previous command
// This tracks the current command index via an internal variable.
const char *XRVCClientCommandParser::RetrieveCommand(const bool getNext)
{
    const int historyVectorSize = static_cast<int>(m_commandHistoryVector.size());
    const char *pRetVal = "";   // assume no more commands in the requested direction
    const int direction = (getNext ? 1 : -1);
    if (m_commandRecallIndex >= 0)   // any commands entered yet?
    {
        m_commandRecallIndex += direction;
        if (m_commandRecallIndex < 0)
            m_commandRecallIndex = 0;   // return earliest command
        else if (m_commandRecallIndex > historyVectorSize)
        {
            // user went beyond the newest command+1
            m_commandRecallIndex = static_cast<int>(m_commandHistoryVector.size()); // reset to one beyond newest command, which means "clear the line"
        }

        _ASSERTE(m_commandRecallIndex <= historyVectorSize);
        if (m_commandRecallIndex < historyVectorSize)
            pRetVal = *m_commandHistoryVector[m_commandRecallIndex];  // normal command recall
        // else fall through and return empty string: the user wants to clear the recall line
    }
    _ASSERTE(pRetVal != nullptr);
    return pRetVal;
}

//-------------------------------------------------------------------------
// ParserTreeNode leaf node callbacks
//-------------------------------------------------------------------------

//=========================================================================
// Common leaf handler that parses a data value and writes it to the pointer in our XREngineStateWrite, then updates the engine state.
// pTreeNode = ParserTreeNode leaf node to which this handler belongs; e.g., "ThrottleLevel" in chain Set->LeftMain->ThrottleLevel 0.56
// remainingArgv = remaining text arguments (typically number values)
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::EngineLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    // Retrieve our engine enum ID(s)
    const EngineNodeData *pNodeData = static_cast<const EngineNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    // parse our single argument
    const XRVCClient::DataType dataType = pNodeData->dataType;
    const CString &arg = remainingArgv[0];  // this is our only argument
    bool parseStatus;  // populated below
    XRVCClient::Value value;  // populated below
    if (dataType == XRVCClient::DataType::Double)
    {
        parseStatus = ParseValidatedDouble(arg, value.Double, pNodeData->minDblValue, pNodeData->maxDblValue, &statusOut);
    }
    else if (dataType == XRVCClient::DataType::Bool)
    {
        parseStatus = ParseValidatedBool(arg, value.Bool, &statusOut);
    }
    else   // invalid data type (should never happen)
    {
        statusOut.Format("INTERNAL ERROR: invalid DataType: %d", dataType); 
        return false;
    }
    if (!parseStatus)
        return false;  

    // argument is OK; update the state of the first engine
    bool success = pNodeData->xrvcClient.UpdateEngineState(pNodeData->engine1, dataType, value, pNodeData->pValueToSet, statusOut);
    if (success)
    {
        // update the state of the second engine if it is different from the one we just set; e.g., handle "MainBoth"
        if (pNodeData->engine1 != pNodeData->engine2)
            success = pNodeData->xrvcClient.UpdateEngineState(pNodeData->engine2, dataType, value, pNodeData->pValueToSet, statusOut);
    }

    return success;
}

//-------------------------------------------------------------------------
// Set csOut to help string for this node; e.g., "<double> (range -1.0 - 1.0)"
// pTreeNode = ParserTreeNode leaf node to which this handler belongs; e.g., "ThrottleLevel" in chain Set->LeftMain->ThrottleLevel 0.56
//-------------------------------------------------------------------------
void XRVCClientCommandParser::EngineLeafHandler::GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const 
{ 
    const EngineNodeData *pNodeData = static_cast<const EngineNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    const XRVCClient::DataType dataType = pNodeData->dataType;
    if (dataType == XRVCClient::DataType::Double)
        csOut.Format("<double> (range %.4lf - %.4lf)", pNodeData->minDblValue, pNodeData->maxDblValue);
    else if (dataType == XRVCClient::DataType::Bool)
        csOut = "<boolean> (true/on, false/off)";
    else   // invalid data type (should never happen)
        csOut.Format("INTERNAL ERROR: invalid DataType: %d", dataType); 
}

// Returns list of valid tokens for the first leaf node parameter, or nullptr if none
const char **XRVCClientCommandParser::EngineLeafHandler::GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode)
{
    const EngineNodeData *pNodeData = static_cast<const EngineNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    const XRVCClient::DataType dataType = pNodeData->dataType;
    const char **pRetVal = nullptr;
    static const char *s_pOnOff[] = { "on", "off", nullptr };
    if (dataType == XRVCClient::DataType::Bool)
        pRetVal = s_pOnOff;   // autocompletion options for boolean are "on", "off"

    return pRetVal;
}

//======================================

//=========================================================================
// Common leaf handler that parses a data value and writes it to the pointer in our XRSystemStatusWrite, then updates the damage state.
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments (typically number values)
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::DamageStateLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    const DamageStateNodeData *pNodeData = static_cast<const DamageStateNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    // parse our single argument
    const XRVCClient::DataType dataType = pNodeData->dataType;
    const CString &arg = remainingArgv[0];  // this is our only argument
    bool parseStatus;  // populated below
    XRVCClient::Value value;  // populated below
    if (dataType == XRVCClient::DataType::Double)
    {
        parseStatus = ParseValidatedDouble(arg, value.Double, 0.0, 1.0, &statusOut);
    }
    else if (dataType == XRVCClient::DataType::Int)
    {
        // parse the text XRDamageState argument 
        parseStatus = true;
        if (arg.CompareNoCase("offline") == 0)
            value.Int = static_cast<int>(XRDamageState::XRDMG_offline);
        else if (arg.CompareNoCase("online") == 0)
            value.Int = static_cast<int>(XRDamageState::XRDMG_online);
        else
        {
            parseStatus = false;
            statusOut.Format("Invalid parameter: '%s'", arg);
        }
    }
    else   // invalid data type (should never happen)
    {
        statusOut.Format("INTERNAL ERROR: invalid DataType: %d", dataType); 
        return false;
    }
    if (!parseStatus)
        return false;  

    // argument is OK; update the damage state 
    return pNodeData->xrvcClient.UpdateDamageState(dataType, value, pNodeData->pValueToSet, statusOut);
}

//-------------------------------------------------------------------------
// Set csOut to help string for this node; e.g., "<double> (range 0.0 - 1.0)"
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
//-------------------------------------------------------------------------
void XRVCClientCommandParser::DamageStateLeafHandler::GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const 
{ 
    const DamageStateNodeData *pNodeData = static_cast<const DamageStateNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    const XRVCClient::DataType dataType = pNodeData->dataType;
    if (dataType == XRVCClient::DataType::Double)
        csOut = "<double> (range 0.0 - 1.0)";
    else if (dataType == XRVCClient::DataType::Int)
        csOut = "online | offline";
    else   // invalid data type (should never happen)
        csOut.Format("INTERNAL ERROR: invalid DataType: %d", dataType); 
}

// Returns list of valid tokens for the first leaf node parameter, or nullptr if none
const char **XRVCClientCommandParser::DamageStateLeafHandler::GetFirstParamAutocompletionTokens(const ParserTreeNode *pTreeNode)
{
    const DamageStateNodeData *pNodeData = static_cast<const DamageStateNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    const XRVCClient::DataType dataType = pNodeData->dataType;
    const char **pRetVal = nullptr;
    static const char *s_pOnlineOffline[] = { "online", "offline", nullptr };
    if (dataType == XRVCClient::DataType::Int)
        pRetVal = s_pOnlineOffline;   // autocompletion options for XRDamageState are "online", "offline"

    return pRetVal;
}
//======================================


//=========================================================================
// Common leaf handler that parses a door state and sets a door
// pTreeNode = ParserTreeNode leaf node to which this handler belongs; e.g., "DockingPort" in chain Set->DockingPort open
// remainingArgv = remaining text arguments ("open", "closed", etc.)
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::DoorLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    // Retrieve our door enum ID
    const DoorNodeData *pNodeData = static_cast<const DoorNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    // parse our single argument
    const CString &arg = remainingArgv[0];  // this is our only argument
    const XRDoorState doorState = ParseDoorState(arg);    // < 0 == error
    if (doorState < XRDoorState::XRDS_Opening)
    {
        statusOut.Format("Invalid door state: '%s'", arg);
        return false;  
    }

    // argument is OK; update the state of the door 
    return pNodeData->xrvcClient.UpdateDoorState(pNodeData->doorID, doorState, statusOut);
}

// Static method that parses a string into an XRDoorState.
// Returns door ID, -1 if pArg string was invalid.
XRDoorState XRVCClientCommandParser::DoorLeafHandler::ParseDoorState(const char *pArg)
{
    struct StringToEnum { const char *str; XRDoorState state; };
    // this is const static because it never changes
    const static StringToEnum doorStates[] = 
    { 
        { "opening", XRDoorState::XRDS_Opening },
        { "open",    XRDoorState::XRDS_Open    },
        { "closing", XRDoorState::XRDS_Closing },
        { "closed",  XRDoorState::XRDS_Closed  }
        // not yet supported: { "failed",  XRDS_Failed  }
    };

    XRDoorState retVal = (XRDoorState)-1;    // assume param invalid
    for (int i=0; i < (sizeof(doorStates) / sizeof(StringToEnum)); i++)
    {
        const StringToEnum *pDoorState = doorStates + i;
        if (_stricmp(pArg, pDoorState->str) == 0)   // case-insensitive comparison here
        {
            retVal = pDoorState->state;
            break;   // found a match
        }
    }

    return retVal;
}

//=========================================================================
// Common leaf handler that parses a boolean state and invokes a callback
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments (e.g., "on", "off".)
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::EnumBoolLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    // Retrieve our ID (usually an enum value)
    const XRVCClientCommandParser::EnumBoolNodeData *pNodeData = static_cast<const EnumBoolNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    // parse our single argument
    bool state;
    const char *pArg = remainingArgv[0];
    if (!ParseBool(pArg, state))
    {
        statusOut.Format("Invalid boolean value: '%s'", pArg);
        return false;  
    }

    // argument in argValue is OK; invoke the callback to perform the XR work
    return (pNodeData->xrvcClient.*(pNodeData->method))(pNodeData->enumID, state, statusOut);  // pNodeData->xrvcClient is the 'this' object for the callback method
}

//=========================================================================
// Common leaf handler that parses a single integer and invokes a callback to perform the XR work.
// pTreeNode = ParserTreeNode leaf node to which this handler belongs; e.g., "Nav" in chain Set->DockingPort open
// remainingArgv = remaining text arguments (e.g., "1")
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::SingleIntLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const SingleIntNodeData *pNodeData = static_cast<const SingleIntNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    // parse our single argument
    int argValue;
    const char *pArg = remainingArgv[0];
    if (pNodeData->IsBoolArgument())
    {
        bool state;
        if (!ParseValidatedBool(pArg, state, &statusOut))
            return false;
        argValue = (state ? TRUE : FALSE);  // convert bool to BOOL
    }
    else  // normal integer argument
    {
        if (!ParseValidatedInt(pArg, argValue, pNodeData->limitLow, pNodeData->limitHigh, &statusOut))
            return false;
    }

    // argument in argValue is OK; invoke the callback to perform the XR work
    return (pNodeData->xrvcClient.*(pNodeData->method))(argValue, statusOut);  // pNodeData->xrvcClient is the 'this' object for the callback method
}

//-------------------------------------------------------------------------
// Set csOut to help string for this node
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
//-------------------------------------------------------------------------
void XRVCClientCommandParser::SingleIntLeafHandler::GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const 
{ 
    const SingleIntNodeData *pNodeData = static_cast<const SingleIntNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    if (pNodeData->IsBoolArgument())
        csOut = "<boolean> (true/on, false/off)";
    else  // int argument, so show limits
        csOut.Format("<int> (range %d - %d)", pNodeData->limitLow, pNodeData->limitHigh);
}

//=========================================================================
// Common leaf handler that parses a double integer and invokes a callback to perform the XR work.
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments (e.g., "1")
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::SingleDoubleLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const SingleDoubleNodeData *pNodeData = static_cast<const SingleDoubleNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    // parse our single argument
    double argValue;
    const char *pArg = remainingArgv[0];
    if (!ParseValidatedDouble(pArg, argValue, pNodeData->limitLow, pNodeData->limitHigh, &statusOut))
        return false;

    // argument in argValue is OK; invoke the callback to perform the XR work
    return (pNodeData->xrvcClient.*(pNodeData->method))(argValue, statusOut);  // pNodeData->xrvcClient is the 'this' object for the callback method
}

//-------------------------------------------------------------------------
// Set csOut to help string for this node
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
//-------------------------------------------------------------------------
void XRVCClientCommandParser::SingleDoubleLeafHandler::GetArgumentHelp(const ParserTreeNode *pTreeNode, CString &csOut) const 
{ 
    const SingleDoubleNodeData *pNodeData = static_cast<const SingleDoubleNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);

    if ((pNodeData->limitLow == -numeric_limits<double>::max()) && (pNodeData->limitHigh == numeric_limits<double>::max()))
        csOut = "<double> (limits are vessel-specific)";
    else
        csOut.Format("<double> (range %.4lf - %.4lf)", pNodeData->limitLow, pNodeData->limitHigh);
}

//=========================================================================
// Leaf handler for Attitude Hold
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::AttitudeHoldLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const BaseNodeData *pNodeData = static_cast<const BaseNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 4, statusOut))
        return false;   // too few/many arguments
    const int argc = static_cast<int>(remainingArgv.size());

    //
    // parse our arguments
    //
    bool isOn;
    bool holdPitch;   // false = hold AOA
    double targetPitch;
    double targetBank;

    // on/off is mandatory
    if (!ParseValidatedBool(remainingArgv[0], isOn, &statusOut))   
        return false;
    
    bool success;
    if (argc > 1)   
    {
        // user wants to set all the parameters
        if (argc != 4)
        {
            statusOut = "Invalid number of paramters: must have either 1 or 4 parameters.";
            return false;
        }
        
        // Pitch/AoA
        const CString &holdArgv = remainingArgv[1];
        if (holdArgv.CompareNoCase("pitch") == 0)
            holdPitch = true;
        else if (holdArgv.CompareNoCase("aoa") == 0)
            holdPitch = false;
        else
        {
            statusOut.Format("Invalid value for [Pitch/AoA] parameter: '%s'", holdArgv);
            return false;
        }

        // <double>TargetPitch
        if (!ParseValidatedDouble(remainingArgv[2], targetPitch, -85, 85, &statusOut))
        {
            statusOut = "TargetPitch " + statusOut;  // "TargetPitch Value out of range..."
            return false;
        }

        // <double>TargetBank
        if (!ParseValidatedDouble(remainingArgv[3], targetBank, -85, 85, &statusOut))
        {
            statusOut = "TargetBank " + statusOut;  // "TargetBank Value out of range..."
            return false;
        }

        // set all four values
        success = pNodeData->xrvcClient.SetAttitudeHold(isOn, &holdPitch, &targetPitch, &targetBank);
    }
    else
    {
        // just set on/off
        success = pNodeData->xrvcClient.SetAttitudeHold(isOn);
    }

    if (success)
        statusOut = "Successfully set AttitudeHold state.";
    else
        statusOut = "Error setting AttitudeHold state.";

    return success;
}

//=========================================================================
// Leaf handler for Descent Hold
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::DescentHoldLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const BaseNodeData *pNodeData = static_cast<const BaseNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 3, statusOut))
        return false;   // too few/many arguments
    const int argc = static_cast<int>(remainingArgv.size());

    //
    // parse our arguments
    //
    
    bool isOn;
    double targetDescentRate;
    bool autoLand;

    // on/off is mandatory
    if (!ParseValidatedBool(remainingArgv[0], isOn, &statusOut))   
        return false;
    
    bool success;
    if (argc > 1)   
    {
        // user wants to set all the parameters
        if (argc != 3)
        {
            statusOut = "Invalid number of paramters: must have either 1 or 3 parameters.";
            return false;
        }
        
        // <double>TargetDescentRate
        if (!ParseValidatedDouble(remainingArgv[1], targetDescentRate, -1000, 1000, &statusOut))
        {
            statusOut = "TargetDescentRate " + statusOut;  
            return false;
        }

        // <bool>AutoLand
        if (!ParseValidatedBool(remainingArgv[2], autoLand, &statusOut))
        {
            statusOut = "AutoLand " + statusOut;   // "AutoLand Invalid boolean ..."
            return false;
        }

        // set all three values
        success = pNodeData->xrvcClient.SetDescentHold(isOn, &targetDescentRate, &autoLand);
    }
    else
    {
        // just set on/off
        success = pNodeData->xrvcClient.SetDescentHold(isOn);
    }

    if (success)
        statusOut = "Successfully set AttitudeHold state.";
    else
        statusOut = "Error setting AttitudeHold state.";

    return success;
}

//=========================================================================
// Leaf handler for Airspeed Hold
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::AirspeedHoldLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const BaseNodeData *pNodeData = static_cast<const BaseNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 2, statusOut))
        return false;   // too few/many arguments
    const int argc = static_cast<int>(remainingArgv.size());

    //
    // parse our arguments
    //
    
    bool isOn;
    double targetAirspeed;

    // on/off is mandatory
    if (!ParseValidatedBool(remainingArgv[0], isOn, &statusOut))   
        return false;
    
    bool success;
    if (argc == 2)     // user setting target airspeed as well?
    {
        // <double>TargetAirspeed
        if (!ParseValidatedDouble(remainingArgv[1], targetAirspeed, 0, numeric_limits<double>::max(), &statusOut))
        {
            statusOut = "TargetAirspeed " + statusOut;  
            return false;
        }
        // set both values
        success = pNodeData->xrvcClient.SetAirspeedHold(isOn, &targetAirspeed);
    }
    else
    {
        // just set on/off
        success = pNodeData->xrvcClient.SetAirspeedHold(isOn);
    }

    if (success)
        statusOut = "Successfully set AirspeedHold state.";
    else
        statusOut = "Error setting AirspeedHold state.";

    return success;
}

//=========================================================================
// Leaf handler for simple one-argument kill commands; e.g., "AP" and "MWS"
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::SimpleResetLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const BaseNodeData *pNodeData = static_cast<const BaseNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments
    const int argc = static_cast<int>(remainingArgv.size());
    
    //
    // parse our single argument
    //
    const CString &csArg = remainingArgv[0];
    bool success;
    if (csArg.CompareNoCase("Autopilots") == 0)
    {
        // this command has no return status (alway succeeds)
        pNodeData->xrvcClient.ResetAutopilots();
        statusOut = "Autopilots reset.";
        success = true;
    }
    else if (csArg.CompareNoCase("MasterWarning") == 0)
    {
        if (!(success = pNodeData->xrvcClient.ResetMasterWarningAlarm()))
            statusOut = "ResetMasterWarningAlarm failed.";
        else
            statusOut = "Master Warning Alarm reset.";
    }
    else if (csArg.CompareNoCase("Damage") == 0)
    {
        if (!(success = pNodeData->xrvcClient.ResetDamage()))
            statusOut = "ResetDamage failed.";
        else
            statusOut = "All damage reset (cleared).";
    }
    else  // invalid command
    {
        statusOut.Format("Invalid command: '%s'", csArg);
        success = false;
    }
    
    return success;
}

//-------------------------------------------------------------------------
// Executes a command and stores a result message to statusOut.
// command = already-autocompleted command
//-------------------------------------------------------------------------
bool XRVCClientCommandParser::ExecuteCommand(const CString &command, CString &statusOut)
{
    bool success = false;
    if (m_xrvcClient.GetXRVessel() != nullptr)   // valid XR vessel?
    {
        // parse the command string and execute the command
        success = m_commandParserTree->Parse(command, statusOut);
    }

    // check whether this command is identical to the last command on the stack; if so, do not add it again
    const int commandHistoryCount = static_cast<int>(m_commandHistoryVector.size());
    if (commandHistoryCount > 0)
    {
        const CString *pLastCommand = m_commandHistoryVector[commandHistoryCount - 1];
        if (*pLastCommand == command)
            goto exit;  // let's have a single exit point rather than two separate 'return success' lines.
    }

    // save the new command in our command stack; there is no limit on the stack size
    m_commandHistoryVector.push_back(new CString(command));     // clone string and save it

exit:
    ResetCommandRecallIndex();  // reset to most recent command
    return success;
}

//=========================================================================
// Leaf handler for Runscript.
// pTreeNode = ParserTreeNode leaf node to which this handler belongs
// remainingArgv = remaining text arguments
// statusOut = CString to which status message will be written
//=========================================================================
bool XRVCClientCommandParser::RunScriptLeafHandler::Execute(const ParserTreeNode *pTreeNode, vector<CString> &remainingArgv, CString &statusOut)
{
    _ASSERTE(pTreeNode != nullptr);

    const BaseNodeData *pNodeData = static_cast<const BaseNodeData *>(pTreeNode->GetNodeData());  // downcast to actual type
    _ASSERTE(pNodeData != nullptr);
    if (!ValidateArgumentCount(static_cast<int>(remainingArgv.size()), 1, 1, statusOut))
        return false;   // too few/many arguments

    // Verify that the file exists before we bother sending it to the thread
    CString csFilename = remainingArgv[0];
    if (_access_s(csFilename, 0x4) != 0)
    {
        statusOut.Format("Script file not found: %s", csFilename);
        return false;
    }

    // send the script filename to our worker thread for execution
    bool success = XRVCMainDialog::s_pSingleton->ExecuteScriptFile(csFilename);
    if (!success)
        statusOut.Format("Script thread is busy.");  // should never happen, really
    else
        statusOut.Format("Script file '%s' queued for execution.", csFilename);  // this should be replaced very shortly by message from the thread
    
    return success;
}

//-------------------------------------------------------------------------
// Static utility methods
//-------------------------------------------------------------------------

// Returns true if argument count is in range (inclusive); on error, returns false
// and writes a reason to statusOut.
bool XRVCClientCommandParser::ValidateArgumentCount(const int argc, const int minArgs, const int maxArgs, CString &statusOut)
{
    if (argc < minArgs)
    {
        statusOut.Format("Insufficient number of parameters.");
        return false;
    }

    if (argc > maxArgs)
    {
        statusOut.Format("Too many parameters.");
        return false;
    }

    return true;
}


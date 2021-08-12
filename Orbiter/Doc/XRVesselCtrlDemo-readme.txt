===========================================================================
XRVesselCtrlDemo 3.2

Release Date: 11-Aug-2021
  
Copyright 2010-2021 Douglas Beachy; All Rights Reserved.

This software is FREEWARE and may not be sold!

NOTE: You may not redistribute this software or its source code nor use it in any other project without express consent from the author.  

http://www.alteaaerospace.com
mailto:doug.beachy@outlook.com
    
  Requirements:
    Orbiter 2016 or newer

    (Optional) Visual Studio 2015 or newer [Only necessary if you want to recompile the module using the included Visual Studio 2015 project file.]
===========================================================================

-------
SUMMARY
-------

I originally developed this module so I could properly test the XRVesselCtrl 2.0 APIs.  However, I decided to release it so that other add-on developers could use it to view sample code for each XRVesselCtrl API call as well as play around with the XRVesselCtrl APIs in order to see exactly what effect a given API call has.  Please note that unlike other XR software, this product is not designed for use by end users (XR pilots): it is designed to provide sample code for Orbiter add-on developers to demonstrate and test the XRVesselCtrl APIs want to develop their own code that interfaces with XR vessels.  Full source code and Visual Studio 2015 C++ project files are included.

---------
COPYRIGHT
---------

This software copyright 2010-2021 Douglas Beachy.  This software is FREEWARE and may not be sold or distributed for a fee under any circumstances, including a "distribution fee."  You may not redistribute this software or host it on your own Web site; however, you are free to link to my Web page at http://www.alteaaerospace.com.

You may not charge a fee of any kind to use this software, nor may you use this software for any commercial purpose (i.e., where profit is involved), regardless of the terms governing the Orbiter instance on which it is running, without express written permission signed by the author and sent via hardcopy letter or fax; i.e., an email is not sufficient to grant permission.  Please feel free to email me at dougb@alteaaerospace.com if you have any questions.   

This software is provided without any warranty, either expressed or implied.

----------
WHAT'S NEW
----------

XRVesselCtrlDemo 3.1 was compiled using Visual Studio 2015 against Orbiter 2016.

Beyond the new XRVesselCtrlDemo test/demo application, the significant changes to the XRVesselCtrl 3.x APIs over version 1.0 are:

1) A new static XRVesselCtrl::IsXRVesselCtrl(const VESSEL *pVessel) method that allows you to easily and safely check whether a given vessel supports the XRVesselCtrl APIs.

2) A new SetXRSystemStatus(const XRSystemStatusWrite &status) method that allows you to set or clear damage dynamically at runtime.  This will allow developers to write custom damage modules that perform things like "random damage", and you can now repair damage to XR vessels at runtime.  [In the demo application, the command to repair all damage is 'Reset Damage'.]  Note, however, that the Reset Damage call cannot "uncrash" a crashed ship.

3) Several bugfixes made to the XRVesselCtrl APIs.  


------------
INSTALLATION
------------

Unzip the XRVesselCtrlDemo zip file to your Orbiter home directory.  The following files will be installed:

    Doc\XRVesselCtrlDemo-readme.txt  (this file)
    Modules\Plugin\XRVesselCtrlDemo.dll
    Orbitersdk\AlteaAerospace\XRVesselCtrlDemo\<source code and VS 2015 project files>

After you unzip the files:

    1. Bring up your Orbiter launchpad and click "Modules".  
    2. Click the checkbox next to "XRVesselCtrlDemo" in the "Developer resources and samples" section to load the plug-in.

You are now ready to launch an Orbiter scenario.


-----------------------
RUNNING THE APPLICATION
-----------------------

While running Orbiter, press CTRL-F4 to bring up the "Custom Function" pop-up.  Then click "XRVesselCtrlDemo 3.1" and click "OK": the XRVesselCtrlDemo dialog appears.

At this point you can select a vessel in the drop-down and view its XRVesselCtrl status by clicking different engine group and subsystem buttons.  If you want to change an XR vessel's state you may do so by issuing commands in the command window.  You may also execute a sequence of commands by running a script file.  When you run a script, the commands are read and executed just as if you had typed them in the command window.


-------------------------
USING THE XR COMMAND LINE
-------------------------

Notice the "Available Params" box below the command line: that box constantly updates as you type to show you available options and parameters for the command you are typing.  [If you are familiar with working from a Windows Command Prompt you will find it intuitive.]  Here is a list of hotkeys you can use on the command line (this text is also available in-sim by clicking "Help"):

    Left/Right/Home/End = Move cursor
    CTRL-left/right     = Jump to previous/next word
    Up/Down Arrow       = Recall previous/next command
    Tab/SHIFT-Tab       = Autocomplete command token
    Enter               = Execute command
    CTRL-Tab            = Delete last word
    Esc                 = Clear command line
                
    F1/'Help' button will toggle this window open/closed.
    The 'Available Params' box shows valid command tokens as you type.
    Commands are case-insensitive.
    
    Example: "Set Door HoverDoors Opening"  (without the quotes)

The key you will use most often is Tab, which will autocomplete the current command if possible.  [This works the same way as the Windows Command Prompt does.]  If you do not autocomplete each token as you type, the application will try to autocomplete each token automatically when the command is executed.

The rest of the dialog should be self-explanatory except for the "Enable Full-Screen Mode" checkbox: when that box is checked, the "Execute Script File" button is disabled to prevent you from accidentally clicking it when you are running Orbiter in full-screen mode.  The reason is because clicking that button will pop up a Windows file browser dialog, which will switch Orbiter out of full-screen mode.  Therefore, if you are running Orbiter in full-screen mode and want to execute a script file you should only use the 'Runscript' command.  The "Enable Full-Screen Mode" checkbox setting is save in/loaded from the scenario file.


-----------------------------------------------
NOTE REGARDING SETTING/READING XR DAMAGE STATES
-----------------------------------------------

XR vessels currently support between 32 and 34 independently damageable systems depending on the which XR vessel you are accessing.  Some systems such as engines, RCS jets, and wings support fractional damage states between 0.0 (no effect on the vessel at all) and 1.0 (full effect on the vessel).  Other systems such as ailerons and ship doors are either *online* or *offline*: there is no fractional damage state for those subsystems.  Refer to the 'DamageState' subcommands in the section below to see what each damageable subsystem supports.  

All damage states are saved with/loaded from the XR vessel's scenario file just as they have always been.


------------
COMMAND LIST
------------

XRVesselCtrlDemo 1.0 Command List

Set 
    Engine 
        MainBoth 
        MainLeft 
        MainRight 
        HoverBoth 
        HoverFore 
        HoverAft 
        ScramBoth 
        ScramLeft 
        ScramRight 
        RetroBoth 
        RetroLeft 
        RetroRight 
            ThrottleLevel <double>          (range 0.0000 - 1.0000)
            GimbalX <double>                (range -1.0000 - 1.0000)
            GimbalY <double>                (range -1.0000 - 1.0000)
            Balance <double>                (range -1.0000 - 1.0000)
            CenteringModeX <boolean>        (true/on, false/off)
            CenteringModeY <boolean>        (true/on, false/off)
            CenteringModeBalance <boolean>  (true/on, false/off)
            AutoMode <boolean>              (true/on, false/off)
            DivergentMode <boolean>         (true/on, false/off)

    Door 
        DockingPort     opening  open  closing  closed 
        ScramDoors      opening  open  closing  closed 
        HoverDoors      opening  open  closing  closed 
        Ladder          opening  open  closing  closed 
        Gear            opening  open  closing  closed 
        RetroDoors      opening  open  closing  closed 
        OuterAirlock    opening  open  closing  closed 
        InnerAirlock    opening  open  closing  closed 
        AirlockChamber  opening  open  closing  closed 
        CrewHatch       opening  open  closing  closed 
        Radiator        opening  open  closing  closed 
        Speedbrake      opening  open  closing  closed 
        APU             opening  open  closing  closed 
        CrewElevator    opening  open  closing  closed 
        PayloadBayDoors opening  open  closing  closed 

    Light 
        Nav    on/true  off/false
        Beacon on/true  off/false
        Strobe on/true  off/false

    StdAutopilot 
        KillRot      on/true  off/false
        Prograde     on/true  off/false
        Retrograde   on/true  off/false
        Normal       on/true  off/false
        AntiNormal   on/true  off/false
        LevelHorizon on/true  off/false
        Hover        on/true  off/false

    XRAutopilot 
        AttitudeHold on/off  [Pitch/AOA  <double>TargetPitch  <double>TargetBank]
        DescentHold  on/off  [<double>TargetDescentRate]  [<bool>AutoLandMode]
        AirspeedHold on/off  [<double>TargetAirspeed]

    DamageState 
        LeftWing         <double> (range 0.0 - 1.0)
        RightWing        <double> (range 0.0 - 1.0)
        LeftMainEngine   <double> (range 0.0 - 1.0)
        RightMainEngine  <double> (range 0.0 - 1.0)
        LeftSCRAMEngine  <double> (range 0.0 - 1.0)
        RightSCRAMEngine <double> (range 0.0 - 1.0)
        ForeHoverEngine  <double> (range 0.0 - 1.0)
        AftHoverEngine   <double> (range 0.0 - 1.0)
        LeftRetroEngine  <double> (range 0.0 - 1.0)
        RightRetroEngine <double> (range 0.0 - 1.0)
        
        ForwardLowerRCS           <double> (range 0.0 - 1.0)
        AftUpperRCS               <double> (range 0.0 - 1.0)
        ForwardUpperRCS           <double> (range 0.0 - 1.0)
        AftLowerRCS               <double> (range 0.0 - 1.0)
        ForwardStarboardRCS       <double> (range 0.0 - 1.0)
        AftPortRCS                <double> (range 0.0 - 1.0)
        ForwardPortRCS            <double> (range 0.0 - 1.0)
        AftStarboardRCS           <double> (range 0.0 - 1.0)
        OutboardUpperPortRCS      <double> (range 0.0 - 1.0)
        OutboardLowerStarboardRCS <double> (range 0.0 - 1.0)
        OutboardUpperStarboardRCS <double> (range 0.0 - 1.0)
        OutboardLowerPortRCS      <double> (range 0.0 - 1.0)
        AftRCS                    <double> (range 0.0 - 1.0)
        ForwardRCS                <double> (range 0.0 - 1.0)
        
        LeftAileron     online | offline
        RightAileron    online | offline
        LandingGear     online | offline
        DockingPort     online | offline
        RetroDoors      online | offline
        TopHatch        online | offline
        Radiator        online | offline
        Speedbrake      online | offline
        PayloadBayDoors online | offline
        CrewElevator    online | offline

    Other 
        SecondaryHUDMode      <int> (range 0 - 5)
        SetTertiaryHUDState   <boolean> (true/on, false/off)
        RCSDockingMode        <boolean> (true/on, false/off)
        ElevatorEVAPortActive <boolean> (true/on, false/off)

Reset                Autopilots | MasterWarning | Damage
Runscript            <filepath\filename>
ShiftCenterOfGravity <double> (limits are vessel-specific)


---------------------------------
VIEWING/COMPILING THE SOURCE CODE
---------------------------------

This C++ module was built and compiled using Microsoft Visual Studio 2015, so if you want to use the included project files you will need Visual Studio 2015 or newer.  [In theory it should work with the Express edition as well, but I have not tested it with that.]  You should also be able to compile the source under a newer version of Visual Studio.  In any case, you do NOT need to recompile the source code in order to run the application.  

For XRVesselCtrl reference purposes the only two source files you need to look at are XRVesselCtrl.h, which defines the API, and XRVCClient.cpp, which actually makes all the XRVesselCtrl API calls in the module execpt for XRVesselCtrl::IsXRVesselCtrl(pOrbiterVessel), which is called from XRVCMainDialog.cpp.  


---------------
REPORTING A BUG
---------------

If you find a bug in the XRVesselCtrl APIs, its implementation in an XR vessel, or in the demo application, please drop me an email at dbeachy@speakeasy.net and I'll do my best to get it fixed in the next release.  However, please do try reproduce the problem in a clean Orbiter installation first so we can eliminate the possibility that a third-party add-on is causing the problem.

Happy Orbiting!

-- end --

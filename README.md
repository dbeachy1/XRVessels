# XR Vessels
XR Series vessel add-ons for Orbiter Space Flight Simulator

## License

The XR vessels and associated utility projects are open source and licensed under the GNU General Public License v3.0. Refer to the [LICENSE](./LICENSE) file for details.

## Installing and Flying the XR Vessels

Refer to the [XR Flight Operations Manual](./XRVessels/XR%20Flight%20Operations%20Manual.pdf) for details about how to install and fly the XR vessels in Orbiter.

## Building the XR Vessels 

You do not need to build the XR vessels in order to use them with Orbiter. However, if you want to build the XR vessels from the source, follow the steps below. These instructions assume you are building both the 32-bit and 64-bit versions. However, you may build only one version if you prefer.

1. Install Visual Studio 2019 from https://visualstudio.microsoft.com/downloads/.
3. Download and install (or build) Orbiter 2016 or later from either https://github.com/mschweiger/orbiter or http://orbit.medphys.ucl.ac.uk/download.html.
4. Clone the XRVessels repository from GitHub to your local machine with:
```bash
git clone git@github.com:dbeachy1/XRVessels.git
```
or
```bash
git clone https://github.com/dbeachy1/XRVessels.git
```

If you're looking for an excellent GUI that makes working with Git easier, I recommend [Tower](https://www.git-tower.com/).

5. Create six environment variables, either in your Windows environment settings or by adding them to `XRVessels\GlobalShared.props`.

* `ORBITER_ROOT` => your 32-bit Debug* Orbiter root folder
* `ORBITER_ROOT_X64` => your 64-bit Debug* Orbiter root folder
* `ORBITER_ROOT_RELEASE` => your 32-bit Release Orbiter root folder
* `ORBITER_ROOT_RELEASE_X64` => your 64-bit Release Orbiter root folder
* `ORBITER_EXE` => `path\filename` relative to Orbiter root folder of your preferred 32-bit Orbiter executable; e.g., `orbiter.exe`
* `ORBITER_EXE_X64` => `path\filename` relative to Orbiter root folder of your preferred 64-bit Orbiter executable; e.g., `Modules\Server\orbiter.exe`

6. Install or build 32-bit Debug* Orbiter to `%ORBITER_ROOT%`.
7. Install or build 64-bit Debug* Orbiter to `%ORBITER_ROOT_X64%`.
8. Install or build 32-bit Release Orbiter to `%ORBITER_ROOT_RELEASE%`.
9. Install or build 64-bit Release Orbiter to `%ORBITER_ROOT_RELEASE_X64%`.

\* NOTE: you can always compile and test debug (as well as release) versions of the XR vessels against _release_ builds of Orbiter, so can always set `ORBITER_ROOT` to match `ORBITER_ROOT_RELEASE` and `ORBITER_ROOT_X64` to match `ORBITER_ROOT_RELEASE_X64` if you prefer.

10. Download the latest **version 2.0 or later** XR vessels binary packages for all the vessels versions you want to build from either https://www.alteaaerospace.com or here on GitHub. Install the XR vessel packages into each Orbiter instance you will use to test the XR vessels you will build. This is necessary so that the associated meshes, etc. are installed to their correct locations under Orbiter so that you can run the XR vessel DLLs you will build.

Now you are ready to compile and link the XR Vessels.

11. Bring up Visual Studio 2019 and open the solution `XRVessels\XRVessels.sln`.
12. Set the desired build target (e.g., `Debug x64`) and click `Build -> Rebuild Solution`; this will build all the XR vessel DLLs and copy both the DLLs and the `<vessel name>.cfg` file for each vessel to their proper locations under `%ORBITER_ROOT%`, `%ORBITER_ROOT_64`, `%ORBITER_ROOT_RELEASE%`, or `%ORBITER_ROOT_RELEASE_64` via Post-Build Events. If you get any build errors, double-check that the above environment variables are set correctly and that you restarted Visual Studio 2019 _after_ you defined those environment variables.
13. After the build succeeds, click `Debug -> Start Debugging` to bring up Orbiter under the Visual Studio debugger, then load your desired XR vessel scenario. You can now debug the XR vessels you just built.

## Creating an Installable Zip File for an XR Vessel

**Prerequesite:**

Ensure that you have [WinRar](https://www.win-rar.com/) installed and added to your Windows path environment variable.

**Creating the zip file:**
1. Open the `XRVessels` project in Visual Studio 2019 and rebuild the release version of the XR vessel for which you want to create an installable zip file.
2. In a Command Prompt, CD to `XRVessels\dist` and run the `makedist` command for the desired XR vessel, passing the package version as the first parameter. For example, to assemble a zip file for the XR1 version 2.1, run this:
```
makedistXR1 2.1
```
3. When the batch file completes, navigate to `XRVessels\dist\out\XR1\2.1`. You should see four zip files:
* DeltaGliderXR1-2.1-dll-x64.zip => 64-bit DLL only
* DeltaGliderXR1-2.1-dll-x86.zip => 32-bit DLL only
* DeltaGliderXR1-2.1-x64.zip => 64-bit installable package
* DeltaGliderXR1-2.1-x86.zip => 32-bit installable package
4. Be sure to test your newly created installation package in a clean installation of Orbiter by extracting it to the root Orbiter folder and verifying that it runs normally.

## Obj2Msh

Regarding the `Obj2Msh` C# project in the `Obj2Msh` folder: `Obj2Msh` is a relatively quick-and-dirty utility I originally wrote to convert the XR2's and XR5's meshes from `.obj` format into Orbiter's `.msh` format. It is not needed to build the XRVessels.


## Support
For more information and support regarding Orbiter and the XR vessels, visit https://www.orbiter-forum.com/.

Happy Orbiting!

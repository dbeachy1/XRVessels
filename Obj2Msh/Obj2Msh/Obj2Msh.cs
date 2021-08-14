/**
  Obj2Msh Converter for Orbiter
  Copyright (C) 2007-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// ObjToMsh.cs : convert .obj model to Orbiter .msh
//-------------------------------------------------------------------------

using System;
using System.Collections.Generic;  // uses Generics; legacy System.Collections does not
using System.Text;
using System.IO;

using com.alteaaerospace.Obj2Msh.common;
using com.alteaaerospace.Obj2Msh.drivers;

namespace com.alteaaerospace.Obj2Msh
{
    public class Obj2Msh
    {
        public const string PROGRAM_NAME   = "Obj2Msh";
        public const string VERSION        = "3.1";
        public const string COPYRIGHT_YEAR = "2007-2021";
        public const string RELEASE_DATE   = "14-Aug-2021";
        public const string WEB_PAGE       = "https://www.alteaaerospace.com";

        protected const int DEFAULT_SPECULAR_POWER = 25;

        protected bool    m_flipFaces = true;   // flip is ON by default
        protected bool    m_debugMode = false;
        protected bool    m_noMaterialReording = false;
        protected bool    m_optimizationDebugMode = false;
        protected bool    m_rotateX = false;
        protected string  m_texturePrefix = "";
        protected int     m_specularPower = DEFAULT_SPECULAR_POWER;
        protected bool    m_ignoreMaterialErrors = true;

        /// <summary>
        ///  Main Entry point 
        /// </summary>
        /// <param name="args"></param>
        /// <returns>0 on success, nonzero on error</returns>
        static int Main(string[] args)
        {
            return new Obj2Msh().Run(args);
        }

        /// <summary>
        /// Main run method
        /// </summary>
        /// <param name="args">inputFile [outputFile]</param>
        /// <returns>0 on success, errorcode on error</returns>
        public int Run(string[] args)
        {
            Out("");
            Out(" >>> " + PROGRAM_NAME + " " + VERSION + " <<<");
            Out("Copyright " + COPYRIGHT_YEAR + " Douglas Beachy");
            Out("Licensed under the terms of the GNU General Public License.");
            Out("See https://www.gnu.org/licenses/ for license details.");
            Out("Release Date: " + RELEASE_DATE);
            Out(WEB_PAGE);
            Out("-------------------------------------------------------");
            Out("");

            // process and remove any switches from args
            args = ProcessSwitches(args);

            if ((args.Length < 1) || (args.Length > 2))
            {
                Out("Usage: Obj2Msh [-rotate] [-noflip] [-nr] [-df] [-d] [-tprefix <path>] [-sp <power>] <inputFile> [<outputFile>]");
                Out("");
                Out("  -rotate  = rotate model 90 degrees nose-down around the X axis");
                Out("  -noflip  = do not flip faces to correct for 'inside-out' artifacts (default is to flip)");
                Out("  -nr      = no reordering of transparent groups and materials to be *last* in the msh file");
                Out("  -df      = debug face optmization mode: show face optimization debug information");
                Out("  -d       = debug mode: show debug information (includes -df)");
                Out("  -tprefix <path>    = texture prefix path; e.g., foo.dds is stored as XR5Vanguard\\foo.dds in the Orbiter msh file");
                Out("  -sp <power>        = set specular power; if not set, default=" + DEFAULT_SPECULAR_POWER);
                Out("  -logMaterialErrors = show a warning when a single group contains multiple materials.");
                Out("");
                Out("To add a whitespace to a group label, use 'xx' in your group name; e.g., 'dg_intxx0' in the obj becomes 'dg_int 0' in the msh.");
                Out("");
                Out("If outputFile is omitted, the default is <inputFilename>.msh.");
                Out("To specify material orverride settings, create a filename named <inputFile>.materialoverride; e.g., 'deltaglider221.obj.materialoverride'.");
                Out("");
                Out("Switches are case-insensitive and may occur anywhere on the command line.");
                return 1;
            }

            // parse normal non-switch parameters
            string inputFilespec = args[0];

            // build the default outputFilespec
            string outputFilespec = inputFilespec;			// assume no "."
            int dotIndex = inputFilespec.IndexOf(".");
            if (dotIndex >= 0)
            {
                string inputFilename = inputFilespec.Substring(0, dotIndex);  // "foo" from "foo.msh"
                outputFilespec = inputFilename + ".msh";	 // "foo.msh"
            }

            // if the user specified an output filespec, use that instead
            if (args.Length >= 2)
                outputFilespec = args[1];

            Out("Converting " + inputFilespec + " --> " + outputFilespec + "...");
            int retCode;
            try
            {
                retCode = ConvertObjToMsh(inputFilespec, outputFilespec);
            }
            catch (IOException ex)
            {
                Out("I/O error reading/writing file(s): " + ex.Message);
                return 2;
            }
            catch (ArgumentException ex)     // invalid file format
            {
                Out(ex.StackTrace);
                Out("Fatal error: " + ex.Message);
                return 3;
            }
            // any other exceptions will halt execution anyway

            Out("DONE!  Exitcode=" + retCode);
            return retCode;
        }

        /// <summary>
        /// Process any switches in the supplied arguments and remove them so that only normal arguments remain.
        /// If an invalid switch is found, an error is displayed and the program exits.
        /// </summary>
        /// <param name="args">command-line arguments</param>
        /// <returns>array of non-switch arguments from args</returns>
        protected string[] ProcessSwitches(string[] args)
        {
            List<string> outArgs = new List<string>();    // (string) values of non-switch arguments

            // Note: we need to use an index (i) in this loop
            for (int i=0; i < args.Length; i++)
            {
                string arg = args[i].ToLower();
                bool isSwitch = true; // assume valid switch
                
                switch (arg)
                {
                    case "-rotate":
                    {
                        m_rotateX = true;
                        Out("Model will be rotated 90 degrees around the X axis (Y=-Z, Z=Y).");
                        break;
                    }
                    
                    case "-noflip":
                    {
                        m_flipFaces = false;
                        Out("Faces will not be flipped.");
                        break;
                    }
                    
                    case "-d":
                    {
                        m_debugMode = true;
                        Out("Debug mode enabled.");
                        break;
                    }
                    
                    case "-df":
                    {
                        m_optimizationDebugMode = true;
                        Out("Face Optimization debug mode enabled.");
                        break;
                    }
                    
                    case "-nr":
                    {
                        m_noMaterialReording = true;
                        Out("Material and Group Reordering disabled; order will not be changed.");
                        break;
                    }
                    
                    case "-tprefix":
                    {
                        // get the following value as the prefix itself
                        int i2 = i + 1;
                        if ((i2 == args.Length) || (args[i2].StartsWith("-")))
                        {
                            Out("");
                            Out("Error: missing prefix value for -tprefix switch.");
                            Environment.Exit(10);
                        }

                        m_texturePrefix = args[i2];
                        i++;    // skip the parm we just parsed
                        Out("Using texture path prefix: " + m_texturePrefix);
                        break;
                    }
                    
                    case "-sp":
                    {
                        // get the following value as the specular power
                        int i2 = i + 1;
                        if ((i2 == args.Length) || (args[i2].StartsWith("-")))
                        {
                            Out("");
                            Out("Error: missing specular power number for -sp switch.");
                            Environment.Exit(11);
                        }

                        string specularPowerstring = args[i2];
                        try
                        {
                            m_specularPower = Int32.Parse(specularPowerstring);
                        }
                        catch (FormatException ex)
                        {
                            Out("");
                            Out("Error parsing -sp value: " + ex);
                            Environment.Exit(11);
                        }

                        i++;    // skip the parm we just parsed
                        Out("Using specular power: " + m_specularPower);
                        break;
                    }
                    
                    case "-logmaterialerrors":
                    {
                        m_ignoreMaterialErrors = false;
                        Out("Logging a warning when a material error is found.");
                        break;
                    }
                    
                    default:
                    {
                        if (arg.StartsWith("-"))
                        {
                            Out("");
                            Out("Error: invalid switch: '" + arg + "'.  Enter '" + PROGRAM_NAME + "' with no parameters for help.");
                            Environment.Exit(11);
                        }
                        else
                            isSwitch = false;
                        break;
                    }
                    
                }  // end of switch
                if (!isSwitch)
                    outArgs.Add(arg);       // this is a normal argument
            }   // end of main loop

            // return the new args
            return outArgs.ToArray();
        }

        /// <summary>
        /// Convert the specified obj file to msh format
        /// </summary>
        /// <param name="inputFilespec">obj file (input)</param>
        /// <param name="outputFilespec">msh file (output)</param>
        /// <returns>exit code: 0 = success</returns>
        /// <exception cref="System.IO.IOException">Thrown if fatal error occurs</exception>
        public int ConvertObjToMsh(string inputFilespec, string outputFilespec)
        {
            // instantiate our readers and writers
            ThreeDReader reader = new ObjThreeDReader(inputFilespec);
            ThreeDWriter writer = new MshThreeDWriter(outputFilespec, reader);

            // perform the conversion
            int warningCount = Convert(reader, writer);
            return warningCount;     // warning count is the error code
        }

        /// <summary>
        /// Convert an input file to and output file
        /// </summary>
        /// <param name="reader">driver to read the input file</param>
        /// <param name="writer">driver to write the output file</param>
        /// <returns>exit code; 0 = success, non-zero = total warnings</returns>
        /// <exception cref="System.IO.IOException">Thrown if fatal error occurs</exception>
        internal int Convert(ThreeDReader reader, ThreeDWriter writer)
        {
            // track the times
            long startTime = DateTime.Now.Ticks;

            // reset the static material indices since we are parsing a new input file
            Material.ResetMaterialAndTextureIndices();

            // propagate switch values to the reader 
            reader.DebugMode = m_debugMode;
            reader.SpecularPower = m_specularPower;
            reader.IgnoreMaterialErrors = m_ignoreMaterialErrors;

            // propagate switch values to the writer
            writer.DebugMode = m_debugMode;
            writer.NoMaterialReordering = m_noMaterialReording;
            writer.OptimizationDebugMode = m_optimizationDebugMode;
            writer.RotateX = m_rotateX;
            writer.TexturePrefix = m_texturePrefix;

            // read and parse the input file
            Out("Parsing input file...");
            int readWarningCount = reader.Read();
            Out("Read complete, " + readWarningCount + " warning(s).");
            float readTime = (float)(DateTime.Now.Ticks - startTime) / 10e6f;  // each tick is 1/10-millionth of a second
            int linesRead = reader.LinesRead;
            int linesReadPerSecond = (int)((double)linesRead / readTime);   // integer resolution is fine here
            Out("Read " + linesRead + " lines from the input file.");
            Out("Parse time: " + readTime + " seconds (" + linesReadPerSecond + " lines per second).");
        
            // flip the faces if requested
            if (m_flipFaces)
            {
                Out("");
                Out("Flipping all faces to correct for 'inside-out' artifacts...");
                reader.FlipFaces();
                Out("Flip Complete.");
            }

            // write the output file
            long writeStartTime = DateTime.Now.Ticks;
            Out("");
            Out("Writing output file...");
            int writeWarningCount = writer.Write();
            Out("Write complete, " + writeWarningCount + " warning(s).");
            float writeTime = (float)(DateTime.Now.Ticks - writeStartTime) / 10e6f;  // each tick is 1/10-millionth of a second
            long linesWritten = writer.LinesWritten;
            int linesWrittenPerSecond = (int)((double)linesWritten / writeTime);   // integer resolution is fine here
            Out("Wrote " + linesWritten + " lines to the msh file.");
            Out("Write time: " + writeTime + " seconds (" + linesWrittenPerSecond + " lines per second).");

            int totalWarnings = reader.WarningCount + writer.WarningCount;
            if (totalWarnings == 0)
            {
                Out("");
                Out("Conversion complete (0 warnings)");
            }
            else
            {
                Out("");
                Out("Warning: conversion complete, but " + reader.WarningCount + " read warning(s) and " +
                    writer.WarningCount + " write warning(s) detected.");
            }

            float totalTime = (float)(DateTime.Now.Ticks - startTime) / 10e6f;  // each tick is 1/10-millionth of a second
            Out("Total conversion time: " + totalTime + " seconds.");

            return totalWarnings;   
        }

        /// <summary>
        /// Output a line to the console, adding a trailing newline
        /// </summary>
        /// <param name="line"></param>
        protected void Out(string line)
        {
            Console.WriteLine(line);
        }
    }
}

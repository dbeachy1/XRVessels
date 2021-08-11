//-------------------------------------------------------------------------
// ThreeDWriter.cs : Abstract base class defining a 3D file writer.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

using System;

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    ///  Abstract base class defining a 3D file writer.
    /// </summary>
    internal abstract class ThreeDWriter
    {
        protected int          m_warningCount = 0;    // total warnings so far
        protected int          m_reusedFaceCount = 0; // # of duplicate faces optimized out (i.e., not re-written to the output file)
        protected string       m_outputFilespec;
        protected ThreeDReader m_reader;   // contains input data about the mesh
        protected bool         m_debugMode = false;
        protected bool         m_optimizationDebugMode = false;
        protected int          m_lineCount = 0;  // lines written
        protected bool         m_noMaterialReording = false;
        protected bool         m_noFaceOptimization = false;  // if true, do not perform any duplicate face reuse optmization
        protected bool         m_rotateX = false;
        protected string       m_texturePrefix = ""; 

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="outputFilepec">e.g., "foo.msh"</param>
        /// <param name="reader">contains all data about a given mesh </param>
        public ThreeDWriter(string outputFilepec, ThreeDReader reader)
        {
            m_outputFilespec = outputFilepec;
            m_reader = reader;
        }

        /// <summary>
        /// Returns the total warning count (read-only)
        /// </summary>
        public int WarningCount 
        {
            get { return m_warningCount; }
            // no setter
        }

        /// <summary>
        /// Enable or get debug mode 
        /// </summary>
        public bool DebugMode
        {
            set { m_debugMode = value; }
            get { return m_debugMode; }
        }

        /// <summary>
        /// Returns the total number of lines written (read-only)
        /// </summary>
        public int LinesWritten
        {
            get { return m_lineCount; }
            // no setter
        }

        /// <summary>
        /// Enable/disable reordering of transparent materials to be LAST in the mesh (write-only)
        /// </summary> 
        public bool NoMaterialReordering
        {
            // no getter
            set { m_noMaterialReording = value; }
        }

        /// <summary>
        /// Show a line to the user
        /// </summary>
        /// <param name="line"></param>
        protected void Out(string line)
        {
            Console.WriteLine(line);
        }

        /// <summary>
        /// Returns the reused face count; i.e., the number of duplicate faces "optimized out"
        /// and not re-written to the output file.  (read-only)
        /// </summary>
        public int ReusedFaceCount
        {
            get { return m_reusedFaceCount; }
            // no setter
        }

        /// <summary>
        /// Enable/disable face optimization mode
        /// </summary>
        public bool OptimizationDebugMode
        {
            get { return m_optimizationDebugMode; }
            set { m_optimizationDebugMode = value; }
        }

        /// <summary>
        /// Perform a right-hand-to-left-hand conversion for a 3-dimensional set of coordinates
        /// </summary>
        /// <param name="c"></param>
        /// <returns>converted coordinates</returns>
        public COORD3 RightHandToLeftHand(COORD3 c)
        {
            // negate the X coordinate
            return new COORD3(-c.X, c.Y, c.Z);
        }

        /// <summary>
        /// Perform a right-hand-to-left-hand conversion for a 2-dimensional set of coordinates;
        /// e.g., for texture coordinates
        /// </summary>
        /// <param name="c"></param>
        /// <returns>converted coordinates</returns>
        public COORD2 RightHandToLeftHand(COORD2 c)
        {
            return new COORD2(c.X, -c.Y + 1);
        }

        /// <summary>
        /// Rotate a set of 3-dimensional coordinates 90 degrees around the X axis:
        /// <pre>
        ///   Y =-Z
        ///   Z = Y
        /// </pre>
        /// </summary>
        /// <param name="c"></param>
        /// <returns>converted coordinates</returns>
        public COORD3 Rotate90DegreesNoseDown(COORD3 c)
        {
            COORD3 retVal = new COORD3();
            retVal.X = c.X;
            retVal.Y = -c.Z;
            retVal.Z = c.Y;

            return retVal;
        }

        /// <summary>
        /// Get/set rotation requested
        /// </summary>
        public bool RotateX
        {
            get { return m_rotateX; }
            set { m_rotateX = value; }
        }


        /// <summary>
        /// Set/get the texture prefix; e.g., if passing "XR5Vanguard", 
        /// foo.dds is stored as XR5Vanguard\\foo.dds in the Orbiter msh file.
        /// Prefix does not include a trailing backslash; may be null or empty
        /// </summary>
        public string TexturePrefix
        {
            get { return ((m_texturePrefix.Length == 0) ? "" : m_texturePrefix + "\\"); }
            set 
            {
                if (value == null)
                    value = "";

                m_texturePrefix = value;
            }
        }

        //---------------------------------------------------------------------
        // Abstract methods begin here
        //---------------------------------------------------------------------

        /// <summary>
        /// Write the 3D output file using the existing state of our 3D reader.
        /// </summary>
        /// <returns>warning count; 0 = success</returns>
        /// <exception cref="System.IO.IOException">Thrown if fatal error occurs</exception>
        public abstract int Write();
    }
}

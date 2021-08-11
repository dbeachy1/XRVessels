//-------------------------------------------------------------------------
// ThreeDReader.cs : Abstract base class defining a 3D file reader.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    /// Abstract base class defining a 3D file reader.
    /// </summary>
    internal abstract class ThreeDReader
    {
        // mesh state data common to all readers
        private readonly string              m_inputFilespec;      // "foo.obj", etc.
        private Dictionary<string, Material> m_materialsMap = new Dictionary<string, Material>();  // key = material name, value = material value
        private List<Material>               m_orderedMaterialList = new List<Material>();         // materials in the order the were added
        private Dictionary<string, Group>    m_groupMap = new Dictionary<string, Group>();         // key=groupName, value=group itself : contains all our group objects
        private SortedList<int, Group>       m_orderedGroupList = new SortedList<int, Group>();    // (Group) objects sorted; key=int
        private List<COORD3>                 m_vertexList = new List<COORD3>();   // list of all our vectex (COORD3) objects in the order they were read
        private List<COORD3>                 m_normalList = new List<COORD3>();   // list of all our normal (COORD3) objects in the order they were read
        private List<COORD2>                 m_textureList = new List<COORD2>();  // list of all our texture (COORD2) objects in the order they were read

        protected int     m_unorderedGroupIndex = 100000;  // unordered groups are inserted in m_orderedGroupList
        protected Group   m_activeGroup = null;   
        protected bool    m_debugMode = false;
        protected int     m_linesRead = 0;      // always applies to the *last* file read
        protected int     m_specularPower = 0;
        protected bool    m_ignoreMaterialErrors = true;  // propagated from our caller

        private Stack<FileStackEntry> m_activeFileStack = new Stack<FileStackEntry>();  // (FileStackEntry) list of active file statistics; the end of the stack is the currently active file entry.
    
        // these values alway apply to the current active file; the subclass is free to use them
        protected string m_activeFilename = null;   // "foo.msh"
        protected int    m_warningCount = 0;        // total warnings so far
        protected int    m_currentLineNumber = 0;   // current line number being parsed
        protected string m_currentLineText = null;  // current line being parsed

        /// <summary>
        /// Inner class to save/restore active file state
        /// </summary>
        protected class FileStackEntry
        {
            // Note: m_warningCount is global for all files and should not be saved/restored
            public readonly string FILENAME;
            public readonly int    CURRENT_LINE_NUMBER;
            public readonly string CURRENT_LINE_TEXT;

            public FileStackEntry(string filename, int currentLineNumber, string currentLineText)
            {
                FILENAME = filename;
                CURRENT_LINE_NUMBER = currentLineNumber;
                CURRENT_LINE_TEXT = currentLineText;
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="inputFilespec">file to be read</param>
        public ThreeDReader(string inputFilespec)
        {
            m_inputFilespec = inputFilespec;
        }

        /// <summary>
        /// Sets a new file to be active; used for fomatting warning messages.
        /// You must invoke <code>popActiveFile</code> before exiting your method.
        /// </summary>
        /// <param name="filename">filename e.g., "foo.msh"</param>
        protected void PushActiveFile(string filename)
        {
            // save current stats on the stack
            FileStackEntry entry = new FileStackEntry(m_activeFilename, m_currentLineNumber, m_currentLineText);
            m_activeFileStack.Push(entry);

            // reset for new file
            m_activeFilename = filename;
            m_currentLineNumber = 0;
            m_currentLineText = null;
        }

        /// <summary>
        /// Pops the newest entry on the active file stack; you must invoke this once for every
        /// time you invoke <code>pushActiveFile</code>.
        /// </summary>
        protected void PopActiveFile()
        {
            // update 'lines read' for the file we just finished processing
            m_linesRead = m_currentLineNumber;  // this was the last line read in the current (now-finished) file

            // restore state from the top of the stack
            FileStackEntry se = (FileStackEntry)m_activeFileStack.Pop();
            m_activeFilename =    se.FILENAME;
            m_currentLineNumber = se.CURRENT_LINE_NUMBER;
            m_currentLineText =   se.CURRENT_LINE_TEXT;
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
        /// Retrieve out input filespace; e.g., "foo.obj"
        /// </summary>
        public string InputFilespec
        {
            get { return m_inputFilespec; }
            // no setter
        }

        /// <summary>
        /// Parse a floating point number from a string.
        /// </summary>
        /// <param name="pieces">line pieces; e.g, "vt", "-15.77", "12.4"</param>
        /// <param name="pieceIndex">0-n; index at which the first number occurs</param>
        /// <returns>float value on success, or null if number string was invalid</returns>
        protected float? parseFloat(string[] pieces, int pieceIndex)
        {
            if (verifyPieceLength(pieces, pieceIndex + 1) == false)  // displays warning if line invalid
                return null;   // line invalid

            Single retVal;
            try
            {
                retVal = Single.Parse(pieces[pieceIndex]);
            }
            catch (FormatException ex)
            {
                Warning("Floating point number is invalid: " + ex.Message, "The values will be ignored.");
                return null;
            }

            return retVal;
        }

        /// <summary>
        /// Read a set of two numbers and return a COORD2 object.
        /// If line invalid, a warning is displayed and null is returned.
        /// </summary>
        /// <param name="pieces">line pieces; e.g, "vt", "-15.77", "12.4"</param>
        /// <param name="startingPieceIndex">0-n; index at which the first number occurs</param>
        /// <returns>COORD2 with both parsed values</returns>
        protected COORD2 parseCoord2(string[] pieces, int startingPieceIndex)
        {
            if (verifyPieceLength(pieces, startingPieceIndex + 2) == false)  // displays warning if line invalid
                return null;   // line invalid

            COORD2 retVal = new COORD2();
            try
            {
                retVal.X = Single.Parse(pieces[startingPieceIndex]);
                retVal.Y = Single.Parse(pieces[startingPieceIndex + 1]);
            }
            catch (FormatException ex)
            {
                Warning("Floating point number is invalid: " + ex.Message, "The two values will be ignored.");
                return null;
            }

            return retVal;
        }

        /// <summary>
        /// Read a set of three numbers and return a COORD3 object.
        /// If line invalid, a warning is displayed and null is returned.
        /// </summary>
        /// <param name="pieces">line pieces; e.g, "v", "-15.77", "12.4", "-3.534"</param>
        /// <param name="startingPieceIndex">0-n; index at which the first number occurs</param>
        /// <returns>COORD3 with all three parsed values</returns>
        protected COORD3 parseCoord3(string[] pieces, int startingPieceIndex)
        {
            if (verifyPieceLength(pieces, startingPieceIndex + 3) == false)  // displays warning if line invalid
                return null;   // line invalid

            COORD3 retVal = new COORD3();
            try
            {
                retVal.X = Single.Parse(pieces[startingPieceIndex    ]);
                retVal.Y = Single.Parse(pieces[startingPieceIndex + 1]);
                retVal.Z = Single.Parse(pieces[startingPieceIndex + 2]);
            }
            catch (FormatException ex)
            {
                Warning("Floating point number is invalid: " + ex.Message, "The three values will be ignored.");
                return null;
            }

            return retVal;
        }

        /// <summary>
        /// Read a set of four numbers and return a RGBA object.
        /// If line invalid, a warning is displayed and null is returned.
        /// </summary>
        /// <param name="pieces">line pieces</param>
        /// <param name="startingPieceIndex">0-n; index at which the first number occurs</param>
        /// <returns>RGBA with all four parsed values</returns>
        protected RGBA parseRGBA(string[] pieces, int startingPieceIndex)
        {   
            if (verifyPieceLength(pieces, startingPieceIndex + 4) == false)  // displays warning if line invalid
                return null;   // line invalid

            RGBA retVal = new RGBA();
            try
            {
                retVal.R = Single.Parse(pieces[startingPieceIndex]);
                retVal.G = Single.Parse(pieces[startingPieceIndex + 1]);
                retVal.B = Single.Parse(pieces[startingPieceIndex + 2]);
                retVal.A = Single.Parse(pieces[startingPieceIndex + 3]);
            }
            catch (FormatException ex)
            {
                Warning("Floating point number is invalid: " + ex.Message, "The four values will be ignored.");
                return null;
            }

            return retVal;
        }

        /// <summary>
        /// Read a set of three integers and return an INT3 object.
        /// If line invalid, a warning is displayed and null is returned.
        /// </summary>
        /// <param name="pieces">line pieces; e.g, "0", "1", "2"</param>
        /// <param name="startingPieceIndex">0-n; index at which the first number occurs</param>
        /// <returns>INT3 with all three parsed values</returns>
        protected INT3 ParseInt3(string[] pieces, int startingPieceIndex)
        {
            if (verifyPieceLength(pieces, startingPieceIndex + 3) == false)  // displays warning if line invalid
                return null;   // line invalid

            INT3 retVal = new INT3();
            try
            {
                retVal.I = Int32.Parse(pieces[startingPieceIndex    ]);
                retVal.J = Int32.Parse(pieces[startingPieceIndex + 1]);
                retVal.K = Int32.Parse(pieces[startingPieceIndex + 2]);
            }
            catch (FormatException ex)
            {
                Warning("Integer number is invalid: " + ex.Message, "The three values will be ignored.");
                return null;
            }

            return retVal;
        }

        /// <summary>
        /// Verify that the supplied piece array is of sufficient length.  If not, a warning is displayed.
        /// </summary>
        /// <param name="pieces">array of pieces to be tested</param>
        /// <param name="minPieceLength"></param>
        /// <returns>true piece length is OK, false if not</returns>
        protected bool verifyPieceLength(string[] pieces, int minPieceLength)
        {
            bool retVal = true;

            if (pieces.Length < minPieceLength)
            {
                Warning("Expected at least " + minPieceLength + " piece(s), but only found " + pieces.Length + ".", 
                    "The line will be ignored.");
                retVal = false;
            }

            return retVal;
        }

        /// <summary>
        /// Display a warning to the user and count it.
        /// </summary>
        /// <param name="msg">text message; a "Warning: " prefix will be prepended to this.</param>
        /// <param name="resultingAction">appended to the end of the warning; e.g., "The line will be ignored."</param>
        protected void Warning(string msg, string resultingAction)
        {
            Out("Warning: error on line #" + m_currentLineNumber + " of " + m_activeFilename + ": " +
                msg + " : Line='" + m_currentLineText + "'.  " + resultingAction);
        
            m_warningCount++;
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
        /// Flip all the faces to correct for "inside-out" artifacts.  From page 8 of 3DModel.pdf:
        ///   <pre>
        ///     "If you want to flip the rendered side of a triangle (e.g. to correct for inside out artefacts)
        ///      you need to rearrange the triangle indices in the following way:
        ///          (i,j,k) -> (i,k,j)"
        ///   </pre>
        /// Note: you must invoked this <i>after</i> <code>read</code> was invoked.
        /// </summary>
        public void FlipFaces()
        {
            // iterate through all the faces in all the groups
            foreach (Group group in m_groupMap.Values)
            {
                for (int i = 0; i < group.FaceCount; i++)
                {
                    Face face = group.GetFace(i);
                    face.Flip();
                }
            }
        }

        /// <summary>
        /// Add a group to this mesh.  If the group is already present in this mesh (as determined by its name),
        /// the existing group is returned.
        /// </summary>
        /// <param name="g"></param>
        /// <param name="indexNumber">index of group in ordered list (-1...n); if -1, goes to end of list</param>
        /// <returns>the existing group, if any, or g if no group existed previously</returns>
        /// <exception cref="System.ArgumentException">Thrown if parameter is out-of-range.</exception>
        protected Group AddGroup(Group g, int indexNumber)
        {
            if (indexNumber < -1)
                throw new ArgumentException("Internal error: indexNumber must be >= -1");
            
            string groupName = g.Name;
            Group retVal = null;
            m_groupMap.TryGetValue(groupName, out retVal);
            if (retVal == null)
            {
                // this is a new group
                m_groupMap.Add(groupName, g);
                retVal = g;

                // update our ordered list as well; this MUST stay in sync with group map
                // let's insert the item by its index number
                if (indexNumber == -1)
                {
                    // use specific index so we leave room for all the ordered groups to be ahead of us
                    m_orderedGroupList.Add(m_unorderedGroupIndex++, g);
                }
                else  // use explicit index
                {
                    m_orderedGroupList.Add(indexNumber, g);
                }
            }

            m_activeGroup = retVal;

            return retVal;
        }

        /// <summary>
        /// Condense the ordered group list to eliminate gaps; you should invoke this after all the groups have been 
        /// read.
        /// </summary>
        protected void CondenseOrderedGroupList()
        {
            SortedList<int, Group> condensedList = new SortedList<int, Group>();
            // this iterates through the list in its normal sorted order
            int groupIndex=0;
            foreach (Group g in m_orderedGroupList.Values)
            {
                if (g != null)
                    condensedList.Add(groupIndex++, g);   // all indexes are sequential
            }

            m_orderedGroupList = condensedList;
        }

        /// <summary>
        /// Returns the active Group; if there is no active group, an ArgumentException is thrown. (read-only)
        /// </summary>
        /// <returns></returns>
        protected Group ActiveGroup
        {
            get
            {
                if (m_activeGroup == null)
                {
                    throw new ArgumentException("Error in file '" + m_activeFilename + "' on line " + m_currentLineText +
                        ": found a group-specific command before any active was group set.  Line='" + m_currentLineText + "'.  Processing cannot continue.");
                }

                return m_activeGroup;
            }
        }

        /// <summary>
        /// Returns the group count (read-only)
        /// </summary>
        public int GroupCount
        {
            get { return m_groupMap.Count; }
        }

        /// <summary>
        /// Returns the named group, or null if group does not exist
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        protected Group GetGroupByName(string name)
        {
            Group retVal = null;
            m_groupMap.TryGetValue(name, out retVal);
            return retVal;
        }

        /// <summary>
        /// Returns a group by its index in the ordered list; this is useful
        /// if you need to write the groups out in a special order.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        public Group GetGroupByIndex(int index)
        {
            return m_orderedGroupList.Values[index];
        }

        /// <summary>
        /// Add a material to this mesh.
        /// </summary>
        /// <param name="m"></param>
        protected void AddMaterial(Material m)
        {
            m_materialsMap.Add(m.Name, m);

            // track the materials added in order as well
            m_orderedMaterialList.Add(m);
        }

        /// <summary>
        /// Returns the specified material, or null if material does not exist
        /// </summary>
        /// <param name="materialName"></param>
        /// <returns></returns>
        protected Material GetMaterial(string materialName)
        {
            Material retVal = null;
            m_materialsMap.TryGetValue(materialName, out retVal);
            return retVal;
        }

        /// <summary>
        /// Returns the global material count for this reader (read-only)
        /// </summary>
        /// <returns></returns>
        public int MaterialCount
        {
            get { return m_materialsMap.Count; }
            // no setter
        }

        /// <summary>
        /// Returns total vertex count for all groups (read-only)
        /// </summary>
        /// <returns></returns>
        public int VertexCount
        {
            get { return m_vertexList.Count; }
            // no setter
        }

        /// <summary>
        /// Returns the total face count for all groups (read-only).
        /// NOTE: this call is relatively expensive, so cache it to a local variable first if you need 
        /// to reuse it in the same method.
        /// </summary>
        /// <returns></returns>
        public int FaceCount
        {
            get
            {
                int count = 0;

                // iterate through all the groups
                foreach (Group group in m_groupMap.Values)
                    count += group.FaceCount;

                return count;
            }
            // no setter
        }

        /// <summary>
        /// Returns the ordered group list; this allows for custom reordering by other classes for special cases.
        /// Any changes made to the list by the caller will be reflected in this class.  Note that, unlike materials,
        /// the group index is not persisted in each group object and so you don't need to worry about keep that in
        /// sync with the ordered list here.
        /// </summary>
        /// <returns>SortedList of (Group) objects in the order they were added (the key denotes the order)</returns>
        public SortedList<int, Group> GetOrderedGrouplist()
        {
            return m_orderedGroupList;
        }

        /// <summary>
        /// Retrieve the ordered material list; this allows for custom reordering by other classes for special cases.
        /// Any changes made to the list by the caller will be reflected in this class.
        /// NOTE: if you modify the material objects' indicies in this class, you must reorder the material list here
        /// to match in order to keep everything in sync!
        /// </summary>
        /// <returns>list of (Material) objects in the order there we added.</returns>
        public List<Material> GetOrderedMaterialList()
        {
            return m_orderedMaterialList;
        }

        /// <summary>
        /// Retrieve a list of all materials in order of their IDs (1...n).
        /// Will never be null, but may be empty.
        /// </summary>
        /// <returns>list of (Material) objects ordered by ID</returns>
        public Material[] GetMaterialsInOrder()
        {
            // convert list to array
            int materialCount = m_orderedMaterialList.Count;
            return m_orderedMaterialList.ToArray();
        }

        /// <summary>
        /// Returns total texture count for all materials.
        /// Note: this is relatively expensive, so cache it to a local variable first
        /// if you need to reuse it in the same method.
        /// </summary>
        public int TextureCountForMaterials
        {
            get
            {
                int count = 0;

                // iterate through all the materials
                foreach (Material m in m_orderedMaterialList)
                {
                    if (m.HasTexture())
                        count++;
                }
                return count;
            }
            // no setter
        }

        /// <summary>
        /// Add a vertex to the global list
        /// </summary>
        /// <param name="c"></param>
        protected void AddVertex(COORD3 c)
        {
            m_vertexList.Add(c);
        }

        /// <summary>
        /// Retrieve a vertex at the named index (0...n).
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        /// <exception cref="System.ArgumentException">Thrown if index is out-of-range</exception>
        public COORD3 GetVertex(int index)
        {
            if ((index < 0) || (index >= m_vertexList.Count))
                throw new ArgumentException("Invalid vertex index: " + index + " : valid range is 0-" + (m_vertexList.Count-1));

            return (COORD3)m_vertexList[index];
        }

        /// <summary>
        /// Add a normal to the global list
        /// </summary>
        /// <param name="c"></param>
        protected void AddNormal(COORD3 c)
        {
            m_normalList.Add(c);
        }

        /// <summary>
        /// Retrieve a normal at the named index (0...n).
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        /// <exception cref="System.ArgumentException">Thrown if index is out-of-range</exception>
        public COORD3 GetNormal(int index)
        {
            if ((index < 0) || (index >= m_normalList.Count))
                throw new ArgumentException("Invalid normal index: " + index + " : valid range is 0-" + (m_normalList.Count-1));

            return (COORD3)m_normalList[index];
        }

        /// <summary>
        /// Add a set of texture coordinates to the global list
        /// </summary>
        /// <param name="c"></param>
        protected void AddTextureCoords(COORD2 c)
        {
            m_textureList.Add(c);
        }

        /// <summary>
        /// Retrieve a set of texture coordinates at the named index (-1...n).  If -1, it means
        /// "no texture" and so null is returned.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        /// <exception cref="System.ArgumentException">Thrown if index is out-of-range</exception>
        public COORD2 GetTextureCoords(int index)
        {
            if ((index < -1) || (index >= m_textureList.Count))
                throw new ArgumentException("Invalid texture index: " + index + " : valid range is -1 -" + (m_textureList.Count-1));

            if (index < 0)
                return null;    // no texture

            return (COORD2)m_textureList[index];
        }

        /// <summary>
        /// Remove any empty groups (i.e., groups with no faces).  This will not affect group ordering, although
        /// <code>condenseOrderedGroups</code> is invoked to make all the group indexes sequential.
        /// </summary>
        /// <returns>number of empty groups removed</returns>
        public int PurgeEmptyGroups()
        {
            CondenseOrderedGroupList();     // IDs must be sequential for this method to work
            Dictionary<string, Group> validGroupMap = new Dictionary<string, Group>();  // will replace m_groupMap

            SortedList<int, Group> newOrderedList = new SortedList<int, Group>();  // will replace m_orderedGroupList
            for (int i=0; i < GroupCount; i++)
            {
                Group g = GetGroupByIndex(i);
                if (g.FaceCount > 0)
                {
                    // group is used
                    if (!validGroupMap.ContainsKey(g.Name))  // not already in group?
                    {
                        validGroupMap.Add(g.Name, g);  // group is valid
                        newOrderedList.Add(i, g);      // rebuild ordered list as well
                    }
                }
            }
            int emptyGroupCount = m_groupMap.Count - validGroupMap.Count;
            m_groupMap = validGroupMap;             // use only valid groups
            m_orderedGroupList = newOrderedList;    // plug in newly rebuilt list

            return emptyGroupCount;
        }

        /// <summary>
        /// Remove any unused materials.  NOTE: this may change texture indicies as well.
        /// </summary>
        /// <returns>number of empty materials removed</returns>
        public int PurgeUnusedMaterials()
        {
            Dictionary<string, Material> validMaterialsMap = new Dictionary<string, Material>();   // will replace m_materialsMap
            List<Material> newOrderedList = new List<Material>();  // will replace m_orderedMaterialList
            int newMaterialIndex = 1;   // reordered from 1
            int newTextureIndex = 1;    // reordered from 1
            for (int i = 0; i < GroupCount; i++)
            {
                Group g = GetGroupByIndex(i);
                Material m = g.Material;
                if (m != null)
                {
                    // material is used
                    if (!validMaterialsMap.ContainsKey(m.Name))  // not already in list?
                    {
                        validMaterialsMap.Add(m.Name, m);  // material is valid
                        newOrderedList.Add(m);             // rebuild ordered list as well
                        m.MaterialIndex = newMaterialIndex++;

                        // must reorder the textures as well to eliminate any gaps in the indices
                        if (m.TextureIndex >= 1) // anything there?
                            m.TextureIndex = newTextureIndex++;
                    }
                }
            }
            int unusedMaterialsCount = m_materialsMap.Count - validMaterialsMap.Count;
            m_materialsMap = validMaterialsMap;         // use only valid materials
            m_orderedMaterialList = newOrderedList;     // plug in newly rebuilt list

            return unusedMaterialsCount;
        }

        /// <summary>
        /// Returns normal count (read-only)
        /// </summary>
        /// <returns></returns>
        public int NormalCount
        {
            get { return m_normalList.Count; }
            // no setter
        }

        /// <summary>
        /// Returns count of vertices to which a texture is assigned (read-only)
        /// </summary>
        /// <returns></returns>
        public int VertexTextureCount
        {
            get { return m_textureList.Count; }
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
        /// Returns the number of lines read (read-only)
        /// </summary>
        /// <returns></returns>
        public int LinesRead
        {
            get { return m_linesRead; }
            // no setter
        }

        /// <summary>
        /// Write the info file detailing the group order, etc.
        /// Note: be sure to invoke condenseOrderedGroupList first to ensure that there are no gaps in the group list.
        /// </summary>
        /// <param name="meshFilename">msh filename to which this info file relates</param>
        /// <param name="textFilename">text file to be created here</param>
        /// <exception cref="System.IO.IOException">Thrown if I/O error occurs</exception>
        public void WriteInfoFile(string meshFilename, string textFilename)
        {
            using (StreamWriter writer = new StreamWriter(textFilename))
            {
                writer.WriteLine("Created by " + Obj2Msh.PROGRAM_NAME + " " + Obj2Msh.VERSION + ", " + DateTime.Now);
                writer.WriteLine();
                writer.WriteLine("Mesh Filename: " + meshFilename);
                writer.WriteLine("Group List:");
                for (int i = 0; i < GroupCount; i++)
                {
                    Group g = GetGroupByIndex(i);
                    writer.WriteLine("  #" + i + ": " + g.Name + " : material=" + g.Material);
                }
                writer.WriteLine();
                writer.WriteLine("-- end --");
            }
        }

        /// <summary>
        /// Write the meshres.h file for the ship's DLL to use.
        /// Note: be sure to invoke condenseOrderedGroupList first to ensure that there are no gaps in the group list.
        /// </summary>
        /// <param name="meshFilename">msh filename to which this info file relates</param>
        /// <param name="meshresFilename">header file to be created here</param>
        /// /// <exception cref="System.IO.IOException">Thrown if I/O error occurs</exception>
        public void WriteResourceHFile(string meshFilename, string meshresFilename)
        {
            using (StreamWriter writer = new StreamWriter(meshresFilename))
            {
                const string line = "//----------------------------------------------------------------------------";

                writer.WriteLine(line);
                writer.WriteLine("// Created by " + Obj2Msh.PROGRAM_NAME + " " + Obj2Msh.VERSION + ", " + DateTime.Now);
                writer.WriteLine("//");
                writer.WriteLine("// " + Obj2Msh.PROGRAM_NAME + " Copyright " + Obj2Msh.COPYRIGHT_YEAR + " Douglas E. Beachy: All Rights Reserved.");
                writer.WriteLine("//");
                writer.WriteLine("// Mesh Filename: " + meshFilename);
                writer.WriteLine(line);
                writer.WriteLine();

                writer.WriteLine("#pragma once");
                writer.WriteLine();

                writer.WriteLine("// Number of mesh groups");
                writer.WriteLine("#define NGRP " + GroupCount);
                writer.WriteLine();

                writer.WriteLine("// Number of materials");
                writer.WriteLine("#define NMAT " + MaterialCount);
                writer.WriteLine();

                writer.WriteLine("// Number of textures");
                writer.WriteLine("#define NTEX " + TextureCountForMaterials);
                writer.WriteLine();

                writer.WriteLine("// Named mesh groups");
            
                // loop through the groups in order
                for (int i = 0; i < GroupCount; i++)
                {
                    Group g = GetGroupByIndex(i);
                    writer.WriteLine("#define GRP_" + g.Name + " " + i);
                }
                writer.WriteLine("// end of file");
            }
        }

        /// <summary>
        /// Set or get specular power, from 0-100
        /// </summary>
        public int SpecularPower
        {
            get { return m_specularPower; }
            set { m_specularPower = value; }
        }

        /// <summary>
        /// Set 'ignore material errors' flag or 
        /// Returns the 'ignore material errors' flag; if set, if a group specifies more than one material
        /// a warning message will be displayed and the new material will be ignored: only the *first* material
        /// found for that group will be used.
        /// </summary>
        public bool IgnoreMaterialErrors
        {
            get { return m_ignoreMaterialErrors; }
            set { m_ignoreMaterialErrors = value; }
        }

        /// <summary>
        /// Parse a material override file, reading state into our materials map.  
        /// NOTE: each material overridden must already exist in our material map!
        /// </summary>
        /// <remarks>
        ///   <pre>
        ///     Example file entry:
        ///     # ====================
        ///     # MATERIAL lambert19SG
        ///     #   d 1.0 1.0 1.0 1.0
        ///     #   a 0.765 0.765 0.765 1.0
        ///     #   s 0.5 0.5 0.5 1.0 25.0
        ///     #   e 0.0 0.0 0.0 1.0
        ///     # ====================
        ///     # Each of these lines after MATERIAL are optional; if present, the values will replace the values read from the MTL file.
        ///   </pre>
        /// </remarks>
        /// <param name="materialOverrideFilename">e.g., deltaglider221.obj.materialoverride</param>
        /// <returns># of materials overriden</returns>
        /// <exception cref="System.IO.IOException">Thrown on I/O error</exception>
        public int ParseMaterialOverrideFile(string materialOverrideFilename)
        {
            int overrideCount = 0;
        
            // check whether the file exists
            if (!File.Exists(materialOverrideFilename))
                return 0;       // nothing to parse

            PushActiveFile(materialOverrideFilename);

            // material override file exists: parse it
            using (StreamReader reader = File.OpenText(materialOverrideFilename))
            {
                // main line loop
                Material activeMaterial = null;  // last 'MATERIAL' target encountered
                bool skipAllUntilNextMaterial = false;   // if true, skip all d a s e lines until next MATERIAL entry
                for (;;)
                {
                    m_currentLineNumber++;   // this is reused for each file that is parsed

                    string line = reader.ReadLine();
                    if (line == null)
                        break;      // end-of-file

                    // show debug if enabled
                    if (DebugMode)
                        Out("Processing line #" + m_currentLineNumber + ": " + line);

                    line = line.Trim();     // whack any leading or trailing whitespace

                    // save in member variable for various error handling
                    m_currentLineText = line;

                    // skip empty lines and comment lines
                    if ((line.Length == 0) || (line.StartsWith("#")))
                        continue;

                    // split the line into space-separated values and trim each piece
                    string[] pieces = ParseLinePieces(line);

                    //
                    // Parse this line's pieces
                    // 
                    if (pieces.Length < 1)
                        continue;   // empty line

                    string cmd = pieces[0].ToLower(); // e.g., 'material', 'd', 'a', 's', 'e'
                    if (cmd.Equals("material"))
                    {
                        if (pieces.Length != 2)
                        {
                            Warning("Incorrect number of parameters on MATERIAL line", "The material entry will be ignored.");
                            skipAllUntilNextMaterial = true;
                            continue;
                        }

                        string materialName = pieces[1];
                        activeMaterial = GetMaterial(materialName);
                        if (activeMaterial == null)
                        {
                            Warning("Material '" + materialName + "' does not exist in the OBJ file", "The material entry will be ignored.");
                            skipAllUntilNextMaterial = true;
                            continue;
                        }

                        // material OK
                        skipAllUntilNextMaterial = false;  // OK to parse material entries now
                        overrideCount++;   // count # of materials overridden
                        continue;   // all done with this line
                    }

                    // if we reach here, we have a material-specific command
                    if (skipAllUntilNextMaterial)
                    {
                        Warning("No MATERIAL specified for this line", "The line will be ignored.");
                        continue;
                    }

                    switch (cmd)
                    {
                        case "d":  // diffuse
                        {
                            RGBA rgba = parseRGBA(pieces, 1);
                            if (rgba == null)
                                continue;   // line invalid

                            rgba.IS_OVERRIDE = true;
                            activeMaterial.Diffuse = rgba;
                            break;
                        }

                        case "a":  // ambient
                        {
                            RGBA rgba = parseRGBA(pieces, 1);
                            if (rgba == null)
                                continue;   // line invalid

                            rgba.IS_OVERRIDE = true;
                            activeMaterial.Ambient = rgba;
                            break;
                        }

                        case "s":  // specular
                        {
                            float? powerFloat = parseFloat(pieces, 5);
                            if (powerFloat == null)
                                continue;   // line invalid

                            RGBA rgba = parseRGBA(pieces, 1);
                            if (rgba == null)
                                continue;   // line invalid

                            SpecularRGBA srgba = new SpecularRGBA(rgba, (float)powerFloat);
                            srgba.IS_OVERRIDE = true;
                            activeMaterial.Specular = srgba;
                            break;
                        }

                        case "e":  // emissive
                        {
                            RGBA rgba = parseRGBA(pieces, 1);
                            if (rgba == null)
                                continue;   // line invalid

                            rgba.IS_OVERRIDE = true;
                            activeMaterial.Emissive = rgba;
                            break;
                        }
                        
                        default:
                        {
                            Warning("Unknown command token", "The line will be ignored.");
                            // fall through and continue to next line
                            break;
                        }
                    }  // end switch
                }  // end for(;;)
            }  // end using(reader)

            PopActiveFile();
            return overrideCount;
        }

        /// <summary>
        /// Extract the filename from all remaining pieces starting at the specified piece index
        /// </summary>
        /// <param name="pieces">pieces from input line</param>
        /// <param name="firstPieceIndex">piece index where filename starts</param>
        /// <returns>filename, ignoring any leading path information</returns>
        protected string ExtractFilename(string[] pieces, int firstPieceIndex)
        {
            // reconstruct the full file path\name string from the supplied pieces
            StringBuilder fullFilename = new StringBuilder();   // will contain full line; e.g., "C:\foo\bar\mytexture.dds"
            for (int i = firstPieceIndex; i < pieces.Length; i++)
                fullFilename.Append(pieces[i]);

            // now parse the path and extract the filename only
            FileInfo fi = new FileInfo(fullFilename.ToString());
            return fi.Name;
        }

        /// <summary>
        /// Parse a line into space-separated pieces; empty pieces (i.e., two or more consecutive spaces) are ignored.
        /// </summary>
        /// <param name="line">line to parse</param>
        /// <returns>array of non-empty pieces; will never be null, but may be empty</returns>
        protected string[] ParseLinePieces(string line)
        {
            // locate the valid (i.e., non-empty) pieces in the line
            List<string> validPieces = new List<string>();
            string[] pieces = line.Split(new char[] { ' ' });
            foreach (string piece in pieces)
            {
                string trimmedPiece = piece.Trim();
                if (trimmedPiece.Length > 0)
                    validPieces.Add(trimmedPiece);
            }

            // return only valid pieces
            return validPieces.ToArray();
        }

        //---------------------------------------------------------------------
        // Abstract methods begin here
        //---------------------------------------------------------------------

        /// <summary>
        /// Read the 3D input file and store its state internally.
        /// </summary>
        /// <returns>warning count; 0 = success</returns>
        /// <exception cref="System.IO.IOException">Thrown if fatal error occurs</exception>
        public abstract int Read();
    }
}

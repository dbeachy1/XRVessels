//-------------------------------------------------------------------------
// MshThreeDWriter.cs : driver to write an Orbiter .msh file
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

using com.alteaaerospace.Obj2Msh.common;

namespace com.alteaaerospace.Obj2Msh.drivers
{
    internal class MshThreeDWriter : ThreeDWriter
    {
        protected StreamWriter m_writer;      // output (file) writer

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="outputFilespec">e.g., "foo.msh"</param>
        /// <param name="reader">contains all data about a given mesh </param>
        public MshThreeDWriter(string outputFilespec, ThreeDReader reader) : base(outputFilespec, reader)
        {
        }

        /// <summary>
        /// Write the 3D output file using the existing state of our 3D reader.
        /// </summary>
        /// <returns>warning count; 0 = success</returns>
        /// <exception cref="System.IO.IOException">Thrown if fatal error occurs</exception>
        public override int Write()
        {
            // remap any transparent textures and groups if requested
            if (m_noMaterialReording == false)
            {
                ReorderTransparentMaterials();
                ReorderGroups();
            }

            //
            // Write the txt file showing the group order
            //
            string infoFilename = m_outputFilespec + ".txt";
            m_reader.WriteInfoFile(m_outputFilespec, infoFilename);

            //
            // Write the meshres.h file for the ship's DLL to use
            //
            string meshresFilename = m_outputFilespec + ".resource.h";
            m_reader.WriteResourceHFile(m_outputFilespec, meshresFilename);

            // set up to write the msh file
            m_writer = new StreamWriter(m_outputFilespec);  // for writeln method

            //
            // Write the msh file
            //
            try
            {
                // file header
                Writeln("MSHX1");
            
                // group count
                int groupCount = m_reader.GroupCount;
                Writeln("GROUPS " + groupCount);

                // loop through each group and write it in order 
                // (order only matters if 'transparent material reordering' is enabled, which it is by default)
                for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
                {
                    Group group = m_reader.GetGroupByIndex(groupIndex);

                    // LABEL
                    Writeln("LABEL " + group.Name);    // group name

                    // MATERIAL
                    Material groupMaterial = group.Material;    // may be null
                    int materialIndex = 0;  // assume default Orbiter material 0 (white, diffuse, opaque)
                    if (groupMaterial != null)
                        materialIndex = groupMaterial.MaterialIndex;    // 1...n
                    Writeln("MATERIAL " + materialIndex);

                    // TEXTURE
                    int textureIndex = 0;   // assume "no texture defined"
                    if (groupMaterial != null)
                        textureIndex = groupMaterial.TextureIndex;  // note: will be 0 if no texture defined
                    Writeln("TEXTURE " + textureIndex);

                    // NOTE: we cannot write the GEOM line yet because we don't yet know how many vertices are in this group.

                    // 
                    // Loop through each face and write out (to a StringBuffer) each unique *triplet* once and *only* once.  If a triplet
                    // is reused, we will reuse the existing triplet line (via its index in the group).  To do that, we track
                    // each triplet that is written out along with its group write index (0...n).  
                    //
                    StringBuilder vertexOutputBuffer = new StringBuilder(8192);  // assume a big group for efficiency
                    int faceCount = group.FaceCount;
                    int nextGroupTripletIndex = 0;              // incremented each time a unique triplet is written out
                    Dictionary<FaceTriplet, int> tripletIndexMap = new Dictionary<FaceTriplet, int>();    // key=triplet written, value=groupFaceIndex

                    for (int i = 0; i < faceCount; i++)
                    {
                        Face face = group.GetFace(i);

                        // Each face has three lines in the Orbiter msh file (although not necessarily contiguous, since lines (vertices) may be reused).
                        for (int j = 0; j < 3; j++)
                        {
                            // Check whether this triplet (vertex) was already written: there is no sense in writing it twice and then updating
                            // the tripletIndexMap, because then the *first* line we wrote out would never be referenced, since the triplet itself 
                            // ("2/3/4") is the key in the map.  i.e., each triplet is linked to one and only one line in the msh file.
                            //
                            // THIS SECTION NOT PORTED (was disabled in previous version):
                            //     In addition, unless face optimization is disabled we also check for the *values* written out on the line itself
                            //     for duplicates; to do that, we use OptimizedFaceTriplet instead of FaceTriplet objects.  The OptmizedFaceTriplet
                            //     checks the actual coordinate values instead of just the global indicies as the FaceTriplet does.
                            //
                            // Note: a properly-created OBJ file should already check for duplicate face values and never write them out in the
                            // first place (because it is inefficient), but we will check it here as well.
                            FaceTriplet t = face.GetTriplet(j);

                            if (tripletIndexMap.ContainsKey(t))     
                            {
                                if (OptimizationDebugMode || DebugMode)
                                    Out(">>> FaceOptimizer: reusing existing face " + t);

                                m_reusedFaceCount++;
                                continue;           // we already wrote this triplet
                            }

                            // This triplet was not written before; let's update our map so we don't write duplicate lines (vertices) in the mesh file.
                            tripletIndexMap.Add(t, nextGroupTripletIndex++);

                            int globalVertexIndex = t.VertexIndex;    // 0...n
                            int globalNormalIndex = t.NormalIndex;    // 0...n
                            int globalTextureIndex = t.TextureIndex;  // -1 = none

                            COORD3 vertex = m_reader.GetVertex(globalVertexIndex);
                            COORD3 normal = m_reader.GetNormal(globalNormalIndex);
                            COORD2 texture = m_reader.GetTextureCoords(globalTextureIndex);  // may be null

                            // rotate the point 90 degrees around the X axis if requested
                            if (RotateX)
                            {
                                vertex = Rotate90DegreesNoseDown(vertex);
                                normal = Rotate90DegreesNoseDown(normal);
                            }

                            // perform right-hand-to-left-hand conversion (must be done last)
                            vertex = RightHandToLeftHand(vertex);
                            normal = RightHandToLeftHand(normal);
                            if (texture != null)
                                texture = RightHandToLeftHand(texture);
                        
                            // output the line
                            string line = vertex.ToASCII() + " " + normal.ToASCII() + " " + ((texture != null) ? texture.ToASCII() : "") + " \r\n";   // vertex / normal / texture coordinates
                            vertexOutputBuffer.Append(line);
                            m_lineCount++;

                            if (m_debugMode)
                                Out("Writing line #" + m_lineCount + ": " + line);
                        }
                    }

                    // GEOM ; group_label
                    int vertexCount = tripletIndexMap.Count;
                    Writeln("GEOM " + vertexCount + " " + faceCount + " ; " + group.Name);  // in case some custom mesh parsers still expect it on the comment line

                    // Write out the vertex specs we buffered up before; we already counted the lines
                    m_writer.Write(vertexOutputBuffer.ToString());  // do not use WriteLine here: the buffer already has newlines embedded, including the final line

                    // Now write out the faces, which in our case here is a list of triangles (three-sided faces).
                    // Each face is written to one Orbiter mesh line as three vertex indices.
                    // We need to map each triplet (stored with the face) with the *group-local* index number for the triplet
                    // assigned when the triplet was written above.
                    for (int j = 0; j < faceCount; j++)
                    {
                        Face face = group.GetFace(j);

                        // obtain the triplet index number (0...n) for each triplet in the face; each unique triplet was only written once.
                        StringBuilder sb = new StringBuilder();
                        for (int k = 0; k < 3; k ++)
                        {
                            FaceTriplet t = face.GetTriplet(k);
                            int? tripletGroupIndex = tripletIndexMap[t];
                            if (tripletGroupIndex == null)  // should never happen!  This means we never wrote out the triplet above!
                                throw new InvalidOperationException("Internal error: groupFaceIndex == null for face " + face);

                            sb.Append(tripletGroupIndex + " ");  // create three vertice index numbers defining this face (triangle)
                        }
                    
                        // now write out the face line: "0 3 1"
                        Writeln(sb.ToString().Trim());
                    }
                }   // end of group loop

                // MATERIALS  (global material table)
                int materialCount = m_reader.MaterialCount;
                Writeln("MATERIALS " + materialCount);
                Material[] materials = m_reader.GetMaterialsInOrder();  // used for two loops below

                // loop through each material spec and write out its name in order of its unique ID (1...n)
                foreach (Material material in materials)  // length should match materialCount
                    Writeln(material.Name);

                // loop through each material spec and write it in order of its unique ID (1...n)
                foreach (Material material in materials)  // length should match materialCount
                {
                    float averageTransparency = material.AverageTransparency;     // 0...1; will be applied to each channel below
                    Writeln("MATERIAL " + material.Name);  // material name

                    // retrieve the material specs
                    RGBA diffuse = material.Diffuse;   // may be null
                    RGBA ambient = material.Ambient;   // may be null
                    RGBA specular = material.Specular;  // may be null
                    RGBA emissive = material.Emissive;  // may be null

                    // set defaults if any value is null
                    // NOTE: the reader should have prevented these from being null!  This is defensive code here.
                    if (diffuse == null)
                        diffuse = new RGBA(0, 0, 0, 1);

                    if (ambient == null)
                        ambient = new RGBA(0, 0, 0, 1);

                    if (specular == null)
                        specular = new SpecularRGBA(0, 0, 0, 1, 60);

                    if (emissive == null)
                        emissive = new RGBA(0, 0, 0, 1);

                    // If explicit alpha was set via a .materialoverride entry, set the alpha for each channel based on the average of this material.
                    // Otherwise, use our best guess based on the data available from the OBJ file.
                    if (!diffuse.IS_OVERRIDE)
                        diffuse.A = averageTransparency;

                    if (!ambient.IS_OVERRIDE)
                        ambient.A = averageTransparency;

                    if (!specular.IS_OVERRIDE)
                        specular.A = averageTransparency;

                    if (!emissive.IS_OVERRIDE)
                        emissive.A = averageTransparency;

                    // each value is non-null now; write them out
                    Writeln(diffuse.ToASCII());
                    Writeln(ambient.ToASCII());
                    Writeln(specular.ToASCII());
                    Writeln(emissive.ToASCII());
                }

                // TEXTURES (global texture table)
                Writeln("TEXTURES " + m_reader.TextureCountForMaterials);

                // loop through the materials in order again and write out the textures
                // Note: can't make texturePrefix 'const' here (grr...)
                string texturePrefix = TexturePrefix;  // will never be null, but may be empty
                foreach (Material material in materials)  // length should match materialCount
                {
                    if (material.HasTexture())  // if true, texture filename should never be null
                    {
                        string textureFilename = material.TextureFilename;

                        // convert the extension to dds
                        int dotIndex = textureFilename.IndexOf(".");
                        if (dotIndex >= 0)
                        {
                            string rawName = textureFilename.Substring(0, dotIndex);  // "foo" from "foo.bmp"
                            textureFilename = rawName + ".dds";	 // "foo.dds"
                        }
                        else    // no extension yet
                        {
                            textureFilename += ".dds";
                        }
                        Writeln(texturePrefix + textureFilename);  // "XR5Vanguard\foo.dds"
                    }
                }
            }
            finally     // clean up
            {
                m_writer.Dispose();
                m_writer = null;    // just to be tidy so no one can try to use it again
            }

            Out("Reused (optimized) Face Count : " + ReusedFaceCount);

            return m_warningCount;
        }

        /// <summary>
        /// Write a line to the output file
        /// </summary>
        /// <param name="line"></param>
        /// <exception cref="System.IO.IOException">Thrown if I/O error occurs</exception>
        public void Writeln(string line)
        {
            m_writer.WriteLine(line);
            m_lineCount++;

            if (m_debugMode)
                Out("Writing line #" + m_lineCount + ": " + line);
        }

        /// <summary>
        /// Reorder any transparent materials to be at the END of the material order.
        /// </summary>
        protected void ReorderTransparentMaterials()
        {
            int materialCount = m_reader.MaterialCount;
            if (materialCount == 0)
                return;     // unlikely, but possible

            Material[] allMaterials = m_reader.GetMaterialsInOrder();

            //   1) Make a list of all transparent and all opaque materials.
            //   2) Create a new material list that puts all opaque materials first.
            //   3) Assign a new sequential order to each material in the list.
            List<Material> opaqueList = new List<Material>();         // all opaque 
            List<Material> transparentList = new List<Material>();    // all transparent

            foreach (Material m in allMaterials)
            {
                if (m.IsTransparent())
                    transparentList.Add(m);
                else
                    opaqueList.Add(m);
            }

            // Now modify each material's index, setting transparent textures LAST.
            // We also have to rebuild the master orderedMaterialList in the reader here to match.
            List<Material> orderedMaterialList = m_reader.GetOrderedMaterialList();  // reference to the reader's master list
            orderedMaterialList.Clear();        // we will rebuild this below
            int materialIndex = 1;
            foreach (Material m in opaqueList) // opaque textures
            {
                m.MaterialIndex = materialIndex++;
                orderedMaterialList.Add(m);    // update master ordered list as well
            }

            foreach (Material m in transparentList)   // transparent textures
            {
                m.MaterialIndex = materialIndex++;
                orderedMaterialList.Add(m);    // update master ordered list as well
            }
        }

        /// <summary>
        /// Reorder any groups that use transparent materials to be at the END of the group list.
        /// </summary>
        protected void ReorderGroups()
        {
            int groupCount = m_reader.GroupCount;
            if (groupCount == 0)
                return;     // unlikely, but possible

            //   1) Make a list of all groups that use transparent and opaque materials.
            //   2) Create a new group list that puts all opaque materials first.
            //   3) Assign a new sequential order to the groups as tracked by the ThreeDReader class.
            List<Group> opaqueList = new List<Group>();         // all w/opaque materials / no materials
            List<Group> transparentList = new List<Group>();    // all w/transparent materials
            for (int i = 0; i < groupCount; i++)
            {
                Group group = m_reader.GetGroupByIndex(i);
                Material m = group.Material;   // may be null; the default opaque material would be used in this case

                if ((m != null) && (m.IsTransparent()))
                    transparentList.Add(group);
                else
                    opaqueList.Add(group);
            }

            // Now rebuild the master orderedGroupList in the reader here to put all the transparent groups LAST.
            SortedList<int, Group> orderedGroupList = m_reader.GetOrderedGrouplist();  // key=denotes the order in which the group was added
            orderedGroupList.Clear();                    // we will rebuild this below
            int orderedGroupIndex = 0;
            foreach (Group g in opaqueList)        // groups w/opaque or no material
                orderedGroupList.Add(orderedGroupIndex++, g);  // rebuild master ordered list

            foreach (Group g in transparentList)   // groups w/transparent material
                orderedGroupList.Add(orderedGroupIndex++, g);     // rebuild master ordered list
        }
    }
}

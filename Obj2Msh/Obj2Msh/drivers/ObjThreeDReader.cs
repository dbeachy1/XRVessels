//-------------------------------------------------------------------------
// ObjThreeDReader.cs : driver to read a .obj file
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.IO;

using com.alteaaerospace.Obj2Msh.common;

namespace com.alteaaerospace.Obj2Msh.drivers
{
    internal class ObjThreeDReader : ThreeDReader
    {
        protected const string GROUP_WHITESPACE_SEPARATOR = "xx";    // this becomes whitespace in the group label on output
        protected Material m_activeMaterial = null; // material currently being populated by reading MTL file
    
        // default values if an OBJ value is not present
        /* ORG from rather nebulous spec
        protected const RGBA s_rgbaDefaultDiffuse  = new RGBA(0.8f, 0.8f, 0.8f, 1.0f);
        protected const RGBA s_rgbaDefaultAmbient  = new RGBA(0.2f, 0.2f, 0.2f, 1.0f);
        protected const RGBA s_rgbaDefaultSpecular = new RGBA(0.7f, 0.7f, 0.7f, 1.0f);  // spec says 1.0, but that is too high in most cases
        */

        // NEW values so we look decent in Orbiter if the obj file doesn't specify values like it should.  
        // These default values are taken from the default DeltaGlider mesh hull materials.
        protected readonly RGBA s_rgbaDefaultDiffuse  = new RGBA(1.0f,   1.0f,   1.0f,   1.0f);
        protected readonly RGBA s_rgbaDefaultAmbient  = new RGBA(0.765f, 0.765f, 0.765f, 1.0f);
        protected readonly RGBA s_rgbaDefaultSpecular = new RGBA(0.271f, 0.271f, 0.271f, 1.0f);  

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="inputFilespec">file to be read</param>
        public ObjThreeDReader(string inputFilespec) : base(inputFilespec)
        {
        }

        /// <summary>
        /// Read the 3D input file and store its state internally.
        /// </summary>
        /// <returns>warning count; 0 = success</returns>
        /// <exception cref="System.IO.IOException">Thrown if fatal error occurs</exception>
        public override int Read()
        {
            string objFilename = InputFilespec;  // foo.obj
            PushActiveFile(objFilename);

            using (StreamReader reader = File.OpenText(objFilename))
            {
                // main line loop
                int unnamedGroupCount = 0;   // used to construct names for unnamed groups
                int fixupGroupIndex = 0;     // used to create unique fixup group names for a given "normal" group
                string latestNormalGroupName = "";  // set each time a new group is encountered in the obj file
                for (;;)
                {
                    m_currentLineNumber++;

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

                    // all lines must have at least 2 pieces *except* for 'g' (group) command
                    if (pieces.Length < 1)
                        continue;   // empty line
                    string cmd = pieces[0].Trim();
                    if (cmd.ToLower().Equals("g") == false)
                    {
                        if (verifyPieceLength(pieces, 2) == false)      // print warning if piece is missing
                            continue;
                    }

                    // we have a valid line here; parse it
                    if (cmd.ToLower().Equals("mtllib")) // materials library
                    {
                        string mtlFilename = pieces[1];  // filename of materials library
                        ReadMtl(mtlFilename);        // populates m_materialList
                    }
                    else if (cmd.ToLower().Equals("g"))  // group
                    {
                        // get the new group's name, if any
                        string groupName;
                        if (pieces.Length >= 2)
                            groupName = pieces[1];  // group name is present
                        else
                        {
                            // group is unnamed, so assign it a default name
                            groupName = "_UNNAMED_" + unnamedGroupCount;
                            unnamedGroupCount++;
                        }

                        // convert special tag in the name to a whitespace, if present
                        string[] namePieces = groupName.Split(new string[] { GROUP_WHITESPACE_SEPARATOR }, StringSplitOptions.None);
                        if (namePieces.Length > 1)  // do we have a separator?
                            groupName = namePieces[0] + " " + namePieces[1];    // "mygroup 1"

                        // parse the group number, if present
                        namePieces = ParseLinePieces(groupName);
                        int groupIndex = -1;    // assume not present
                        if (namePieces.Length >= 2)    // the second piece is the group index
                        {
                            string indexStr = namePieces[1];
                            try
                            {
                                groupIndex = Int32.Parse(indexStr);
                            }
                            catch (FormatException)
                            {
                                // 2nd piece is not a valid number; ignore it
                            }
                        }

                        // preserve this new group name as the latest 'normal' group name so that any possible fixup groups will be named correctly
                        latestNormalGroupName = groupName;
                        fixupGroupIndex = 0;   // reset to keep fixup names consistent within a group

                        // Check whether this is an existing group to which we are adding data.
                        // (the OBJ spec allows breaking up of group data)
                        Group group = new Group(groupName);
                        group = AddGroup(group, groupIndex);        // Note: if 'group' already exists, the existing group is returned
                    }
                    else if (cmd.ToLower().Equals("usemtl"))  // material tag
                    {
                        // this applies to the active group
                        Group activeGroup = ActiveGroup;  // Note: local 'activeGroup' may change below

                        // check whether this group already has a material specified: Orbiter only supports *one material per group*
                        if (activeGroup.Material != null)
                        {
                            // create a new fixup group on-the-fly
                            fixupGroupIndex++;
                            string fixupGroupName = latestNormalGroupName + "_fixup_" + fixupGroupIndex;
                            
                            // warn the user if requested
                            if (!IgnoreMaterialErrors)
                                Warning("Multiple materials specified for group " + activeGroup, "Forcing an extra group with new material: name=" + fixupGroupName);

                            Group group = new Group(fixupGroupName);
                            activeGroup = AddGroup(group, -1);     // Note: if 'group' already exists, the existing group is returned
                            // fall through and use new activeGroup
                        }

                        // we already verified that we have at least 2 pieces earlier
                        string materialTag = pieces[1];
                        Material m = GetMaterial(materialTag);
                        if (m == null)
                        {
                            Warning("Invalid (unknown) material name specified: '" + materialTag + "'", "The line will be ignored.");
                            continue;
                        }
                        activeGroup.Material = m;
                    }
                    else if (cmd.ToLower().Equals("v"))  // vertex
                    {
                        COORD3 c = parseCoord3(pieces, 1);  // prints warning if invalid
                        if (c == null)
                            continue;

                        // save the new vertex in our global vertex list
                        AddVertex(c);
                    }
                    else if (cmd.ToLower().Equals("vt"))  // vertex texture coordinates
                    {
                        COORD2 c = parseCoord2(pieces, 1);  // prints warning if invalid
                        if (c == null)
                            continue;

                        // save the new vertex in our global vertex list
                        AddTextureCoords(c);
                    }
                    else if (cmd.ToLower().Equals("vn"))  // normal coordinates
                    {
                        COORD3 c = parseCoord3(pieces, 1);  // prints warning if invalid
                        if (c == null)
                            continue;

                        // save the new vertex in our global vertex list
                        AddNormal(c);
                    }
                    else if (cmd.ToLower().Equals("f"))
                    {
                        // Order is: v/vt/vn
                        // If no texture:  v//vn

                        // there may be any number of "1/2/3" pieces to parse here, but we require exactly three per face because Orbiter requires *triangles*.
                        if (pieces.Length != 4)
                        {
                            Warning("Invalid face definition: Orbiter mesh requires each face to contain exactly three vertices each, but face contains " +
                                (pieces.Length - 1) + " vertices.  Open your 3D software and convert each face to triangles; then re-export the model and try again.", "Exiting.");
                            throw new IOException("Invalid face data in OBJ file; exiting.");   // terminate so the user isn't shown thousands of debug lines
                        }

                        // create new face 
                        Face face = new Face();

                        // parse our three face points
                        for (int pieceIndex = 1; pieceIndex < 4; pieceIndex++)  // skip leading "f" piece
                        {
                            // split this piece into three "/"-separated numbers
                            string[] facePieces = pieces[pieceIndex].Split(new char[] { '/' });

                            // trim each piece
                            for (int i=0; i < facePieces.Length; i++)
                                facePieces[i] = facePieces[i].Trim();

                            // if 2nd piece is empty, it means there are no texture coordinates for this face, so set to "-1" so the parse will succeed
                            if (facePieces[1].Length == 0)
                                facePieces[1] = "-1";

                            INT3 i3 = ParseInt3(facePieces, 0); // convert to 3 integers
                            if (i3 == null)
                            {
                                Warning("Invalid face definition at face #" + pieceIndex, "The face will be ignored.");
                                continue;
                            }

                            // convert OBJ 1-based indices to 0-based indices that we use
                            i3.I -= 1;
                            if (i3.J > 0)       // anything to adjust?
                                i3.J -= 1;
                            i3.K -= 1;

                            // add this data point to our new face
                            FaceTriplet t = new FaceTriplet(i3);
                            face.SetTriplet(pieceIndex - 1, t);  

                            // Note: do not validate triplet indicies here; we will check them later when read them.  It is possible that some of the vertices were not
                            // read in yet, since the spec allows them to be spread across the file in different chunks.
                        }

                        // add the new face to our active group
                        ActiveGroup.AddFace(face);
                    }
                    // else ignore all other commands
                } // end of main loop
            }   // end of using block
            

            // read in any material override data that may be present
            string materialOverrideFilename = InputFilespec + ".materialoverride";
            int materialOverrideCount = ParseMaterialOverrideFile(materialOverrideFilename);

            // purge any empty groups; i.e., groups without any faces
            // this will also invoke condenseOrderedGroupList, but there may still be some gaps in group ordering.
            int emptyGroups = PurgeEmptyGroups();

            // purge any unused materials
            int unusedMaterials = PurgeUnusedMaterials();
        
            // condense the ordered group list to eliminate any remaining gaps in the group index numbering
            CondenseOrderedGroupList();

            // print statistics
            Out("");
            Out("Empty Groups       : " + emptyGroups + ((emptyGroups > 0) ? " (purged)" : ""));
            Out("Valid Groups       : " + GroupCount);
            Out("Vertices           : " + VertexCount);
            Out("Vertex Textures    : " + VertexTextureCount);
            Out("Normals            : " + NormalCount);
            Out("Faces              : " + FaceCount);
            Out("Material Overrides : " + materialOverrideCount);
            Out("Unused Materials   : " + unusedMaterials + ((unusedMaterials > 0) ? " (purged)" : ""));
            Out("Valid Materials    : " + MaterialCount);
            Out("Materials w/Texture: " + TextureCountForMaterials);
            Out("");

            PopActiveFile();

            return m_warningCount;
        }

        /// <summary>
        /// Read the MTL file and update our object state with it
        /// </summary>
        /// <param name="mtlFilespec">input file</param>
        /// <returns>warning count</returns>
        /// <exception cref="System.IO.IOException">Thrown if I/O error occurs</exception>
        protected int ReadMtl(string mtlFilespec)
        {
            PushActiveFile(mtlFilespec);

            using (StreamReader reader = File.OpenText(mtlFilespec))
            {
                // main line loop
                for (;;)
                {
                    m_currentLineNumber++;
                    string line = reader.ReadLine();
                    if (line == null)
                        break;      // end-of-file

                    line = line.Trim();     // whack any leading or trailing whitespace

                    // save in member variable for various error handling
                    m_currentLineText = line;

                    // skip empty lines and comment lines
                    if ((line.Length == 0) || (line.StartsWith("#")))
                        continue;

                    // split the line into space-separated values and trim each piece
                    string[] pieces = ParseLinePieces(line);

                    if (verifyPieceLength(pieces, 2) == false)      // print warning if piece is missing
                      continue;

                    // we have a valid line here; parse it
                    string cmd = pieces[0].Trim();
                    if (cmd.ToLower().Equals("newmtl")) // new material definition coming here
                    {
                        string mtlName = pieces[1];  // unique material tag
                        m_activeMaterial = new Material();
                        m_activeMaterial.Name = mtlName;
                        AddMaterial(m_activeMaterial);  // save in our global map
                    }
                    else if (cmd.ToLower().Equals("kd"))  // diffuse
                    {
                        // no alpha stored with this
                        RGBA c = new RGBA(parseCoord3(pieces, 1));
                        // some obj formats specify 0.0 when they mean "use default"
                        if (c.IsZero())
                            c = new RGBA(s_rgbaDefaultDiffuse);   // obj defaults

                        m_activeMaterial.Diffuse = c;
                    }
                    else if (cmd.ToLower().Equals("ka"))  // ambient
                    {
                        // no alpha stored with this
                        RGBA c = new RGBA(parseCoord3(pieces, 1));
                        // some obj formats specify 0.0 when they mean "use default"
                        if (c.IsZero())
                            c = new RGBA(s_rgbaDefaultAmbient);   // obj defaults

                        m_activeMaterial.Ambient = c;
                    }
                    else if (cmd.ToLower().Equals("ks"))  // specular
                    {
                        // no alpha or power stored with this
                        RGBA c = new RGBA(parseCoord3(pieces, 1));
                        float power = 0;        // assume no specular specified
                        // some obj formats specify 0.0 when they mean "use default"
                        if (c.IsZero())
                            c = new RGBA(s_rgbaDefaultSpecular);   // Note: obj defaults are too high (1.0), so let's use 0.50 with a power of zero.
                        else
                            power = SpecularPower;   // specular is present, so use configured power

                        SpecularRGBA specular = new SpecularRGBA(c, power);
                        m_activeMaterial.Specular = specular;
                    }
                    else if (cmd.ToLower().Equals("ke"))  // emissive
                    {
                        // no alpha stored with this
                        RGBA c = new RGBA(parseCoord3(pieces, 1));
                        // no OBJ defaults for this
                        m_activeMaterial.Emissive = c;
                    }
                    else if (cmd.ToLower().Equals("map_kd"))  // texture filename
                    {
                        string filename = ExtractFilename(pieces, 1);
                        m_activeMaterial.TextureFilename = filename;
                    }
                    else if (cmd.ToLower().Equals("tf"))  // transparency color
                    {
                        // no alpha stored with this
                        RGBA c = new RGBA(parseCoord3(pieces, 1));
                        m_activeMaterial.TransparencyColor = c;
                    }
                    // ignore "illum" (illumination mode) for now
                    // ignore "Ni" (index of reflection) for now
                    // ignore any other lines
                } // end of main loop
            }   // end of using block

            // set obj default values for any textures not explicitly specified for each material
            foreach (Material m in GetMaterialsInOrder())  // loop through all materials
            {
                if (m.Diffuse == null)
                    m.Diffuse = new RGBA(s_rgbaDefaultDiffuse);

                if (m.Ambient == null)
                    m.Ambient = new RGBA(s_rgbaDefaultAmbient);

                if (m.Specular == null)
                    m.Specular = new SpecularRGBA(s_rgbaDefaultSpecular, 0.0f);  // power = 0

                // no emissive default
            }

            PopActiveFile();
            return m_warningCount;
        }  
    }  
}  

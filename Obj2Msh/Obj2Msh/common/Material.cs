//-------------------------------------------------------------------------
// Material.cs : Data class encapsulating an material.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

using System;

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    /// Data class encapsulating a material.
    /// </summary>
    internal class Material
    {
        // static values to track the next material and texture indices
        private static int s_nextMaterialIndex = 1;
        private static int s_nextTextureIndex = 1;

        private int m_materialIndex;      // unique material index (1...n) in order of creation; will always be > 0 (set in constructor)
        private int m_textureIndex = 0;   // unique texture index (1...n) in order of creation, or 0 if not set

        // for numerical values, -1 means "not set"
        private string m_name = null;             // "newmtl"  (unique tag)
        private RGBA m_diffuse = null;            // "Kd" 
        private RGBA m_ambient = null;            // "Ka" 
        private SpecularRGBA m_specular = null;   // "Ks" 
        private RGBA m_emissive = null;           // "Ke" 

        private RGBA m_transparencyColor = null;  // "Tf" 
        private string m_textureFilename = null;  // "map_Kd"; e.g., "foo.dds"

        /// <summary>
        /// Reset the material and texture indices; you must invoke this each time
        /// before you read a new obj file.
        /// Note: do not read more than one 3D file at once!  This would 
        /// cause the texture and material indices to get out-of-sync.
        /// </summary>
        public static void ResetMaterialAndTextureIndices()
        {
            s_nextMaterialIndex = s_nextTextureIndex = 1;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public Material()
        {
            m_materialIndex = s_nextMaterialIndex++;
        }

        /// <summary>
        /// Returns our unique material index, from 1...n., or
        /// set a new material index; this is only invoked if the writer needs
        /// to change the order in which the materials are written out, or if some
        /// materials are removed. Value must be >= 1
        /// </summary>
        /// <exception cref="System.ArgumentException">Thrown if parameter is out-of-range.</exception>
        public int MaterialIndex
        {
            get { return m_materialIndex; }
            set 
            {
                if (value < 1)
                    throw new ArgumentException("Internal program error: material index (" + value + ") must be >= 1");

                m_materialIndex = value; 
            }
        }

        /// <summary>
        /// Returns or sets our unique texture index, if any.
        /// If 0, no texture was assigned.  For set, 
        //  this is only invoked if the writer needs
        /// to change the order in which the textures are written out, or if 
        /// some textures are removed (by removing their parent materials).
        /// </summary>
        /// <exception cref="System.ArgumentException">Thrown if parameter is out-of-range.</exception>
        public int TextureIndex
        {
            get { return m_textureIndex; }
            set
            {
                if (value < 1)
                    throw new ArgumentException("Internal program error: texture index (" + value + ") must be >= 1");

                m_textureIndex = value;
            }
        }

        /// <summary>
        /// Returns true if this material has a texture assigned, false if it does not.
        /// </summary>
        /// <returns></returns>
        public bool HasTexture()
        {
            return (m_textureIndex > 0);
        }

        public string Name
        {
            get { return m_name; }
            set { m_name = value; }
        }

        public RGBA Diffuse
        {
            get { return m_diffuse; }
            set { m_diffuse = value; }
        }

        public RGBA Ambient
        {
            get { return m_ambient; }
            set { m_ambient = value; }
        }

        public SpecularRGBA Specular
        {
            get { return m_specular; }
            set { m_specular = value; }
        }

        public RGBA Emissive
        {
            get { return m_emissive; }
            set { m_emissive = value; }
        }

        public RGBA TransparencyColor
        {
            get { return m_transparencyColor; }
            set { m_transparencyColor = value; }
        }

        /// <summary>
        /// For mesh formats that require it: returns the average transparency of 
        /// the RGB color transparency values.  If transparency has not been set, returns 1.0.
        /// Required for Orbiter msh format. (read-only).
        /// Note: this call non-trivial, so if you need to reuse it in the same method you should 
        /// cache it in a local variable first.
        /// </summary>
        /// <returns></returns>
        public float AverageTransparency
        {
            get
            {
                RGBA c = TransparencyColor;
                if (c == null)
                    return 1.0f;

                float avg = (c.R + c.G + c.B) / 3.0f;
                return avg;
            }
            // no setter
        }

        public string TextureFilename
        {
            get { return m_textureFilename; }
            set
            {
                m_textureIndex = s_nextTextureIndex++;  // assign unique index 
                m_textureFilename = value;
            }
        }

        /// <summary>
        /// Returns true if this material is transparent or false if opaque.
        /// </summary>
        /// <returns></returns>
        public bool IsTransparent()
        {
            return (AverageTransparency < 1.0);
        }

        /// <summary>
        /// Display as a string
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            string str = "[name=" + Name + ", transparency=" + AverageTransparency + ", texture=" + ((TextureFilename == null) ? "NONE" : TextureFilename) + "]";
            return str;
        }
    }
}

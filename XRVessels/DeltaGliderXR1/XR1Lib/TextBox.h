// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// TextBox.h
// Draw text in a Windows area; supports newlines via & values.
// ==============================================================

#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include "windows.h"

#include "XR1Colors.h"

#include <string>
#include <vector>

using namespace std;

enum TEXTCOLOR { Normal, Highlighted };

// line of text to be added to buffer
struct TextLine
{
    // Normal constructor
    TextLine(const char *pText, TEXTCOLOR color) 
    {
        this->text = pText;
        this->color = color;
    }

    // NOTE: A copy constructor is required by the 'vector' class.
    TextLine(const TextLine &that) 
    {
        text = that.text;
        color = that.color;
    }

    string text;                // text itself
    TEXTCOLOR color;      // color of line to be rendered

    bool operator=(const TextLine &that) 
    { 
        return ((text == that.text) && (color == that.color));
    }
};

// Manages a group of TextLine objects; this is the primary public object for populating a TextBox
class TextLineGroup
{
public:
    TextLineGroup(const int maxLines);
    virtual ~TextLineGroup();

    int GetLineCount() const { return static_cast<int>(m_lines.size()); }
    void Clear() { m_lines.clear(); }

    // retrives lines in buffer
    const vector<const TextLine *> &GetLines() const { return m_lines; } 

    // retrieves a single line from the buffer
    const TextLine &GetLine(const int index) const { return *m_lines[index]; }

    // Returns how many times AddLines has been invoked; useful to determine whether
    // text has changed since the last check.
    int GetAddLinesCount() const { return m_addLinesCount; }

    // main method that clients will use
    virtual void AddLines(const char *pStr, bool highlighted);

protected:
    void AddLine(const TextLine &textLine);
    const int m_maxLines;
    int m_addLinesCount;   // total # of times AddLines invoked
    vector<const TextLine *> m_lines;  // uses TextLine ptrs for 1) efficiency, and 2) so that vector.erase works
};

//-------------------------------------------------------------------------

class TextBox
{
public:
    TextBox(int width, int height, COLORREF normalTextColor, COLORREF highlightTextColor, COLORREF bgColor, int screenLineCount, const TextLineGroup &textLineGroup);
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    COLORREF GetBackgroundColor() const { return m_bgColor; }
    COLORREF GetNormalColor() const     { return m_normalTextColor; }
    COLORREF GetHighlightColor() const  { return m_highlightTextColor; }
    int GetScreenLineCount() const      { return m_screenLineCount; }
    const TextLineGroup &GetTextLineGroup() const { return m_textLineGroup; }
    
    virtual bool Render(HDC hDC, int topY, HFONT font, int lineSpacing, bool forceRender, int startingLineNumber = -1);
    
protected:
    int m_width, m_height;
    COLORREF m_normalTextColor, m_highlightTextColor, m_bgColor;
    int m_screenLineCount;  // # of text lines on screen
    int m_lastRenderedAddLinesCount;  

    // reference to text lines themselves
    const TextLineGroup &m_textLineGroup;
};


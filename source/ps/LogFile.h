/*
Log File Writer.
by Mark Ellis
mark@markdellis.co.uk

--Overview-- 
	Writes specified output to a formatted html file.

--Usage--
	First open a file for writing using either the constructor
	or the Open() function. Then use the formatting functions to
	write to the HTML file. You may use Close() but the destructor
	will handle all cleanup of the class.

	You can enable a frame at the top of the page that will link to the errors,
	this is enabled by passing true as the 3rd parameter when opening.

--More Info--

	http://wildfiregames.com/0ad/codepit/tdd/logfile.html

*/

#ifndef LOGFILE_H
#define LOGFILE_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "Prometheus.h"
#include <string>
#include <fstream>

using std::string;
using std::ofstream;



//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

// Extra parameters for displaying text
#define PS_SHOW_LINE_NUMBER   PS_DISPLAY_SETTINGS ( __LINE__, __FILE__, __DATE__, PS_DISPLAY_MODE_SHOW_LINE_NUMBER )
#define PS_SHOW_DATE          PS_DISPLAY_SETTINGS ( __LINE__, __FILE__, __DATE__, PS_DISPLAY_MODE_SHOW_DATE )
#define PS_NONE               PS_DISPLAY_SETTINGS ( __LINE__, __FILE__, __DATE__, PS_DISPLAY_MODE_NONE )



//-------------------------------------------------
// Types
//-------------------------------------------------

enum PS_DISPLAY_MODE { PS_DISPLAY_MODE_SHOW_LINE_NUMBER,
					   PS_DISPLAY_MODE_SHOW_DATE,
					   PS_DISPLAY_MODE_NONE };


enum PS_TEXT_ALIGN { PS_ALIGN_LEFT, PS_ALIGN_CENTER, PS_ALIGN_RIGHT };



//-------------------------------------------------
// Declarations
//-------------------------------------------------

class PS_DISPLAY_SETTINGS
{
public:

	PS_DISPLAY_SETTINGS(int Line, char *File, char *Date, PS_DISPLAY_MODE DisplayMode)
	{
		line=Line;
		file=File;
		date=Date;
		displayMode=DisplayMode;
	}

	int line;
	char *file;
	char *date;
	PS_DISPLAY_MODE displayMode;
};




class CLogFile
{

public:

	//Standard Constructor and destructor.
	CLogFile();
	~CLogFile();
	CLogFile(string FileName, string PageTitle, bool WithFrame=false);
	
	//Opens a file for output the 3rd parameter can be set to true to enable the error frame.
	PS_RESULT Open(string FileName, string PageTitle, bool WithFrame=false);

	//Closes the file.
	PS_RESULT Close();
	

	//The following functions edit the html file.

	//Writes a header to the file.
	PS_RESULT WriteHeader(string Text, PS_DISPLAY_SETTINGS displayOptions=PS_NONE);

	//Writes a header to the file. Align can be 0=left, 1=centre, 2=left
	PS_RESULT WriteHeader(string Text, PS_TEXT_ALIGN Align, PS_DISPLAY_SETTINGS displayOptions=PS_NONE);

	//Writes some text to the file.
	PS_RESULT WriteText(string Text, PS_DISPLAY_SETTINGS displayOptions=PS_NONE);

	//Writes some text at a specified alignment.
	PS_RESULT WriteText(string Text, PS_TEXT_ALIGN Align, PS_DISPLAY_SETTINGS displayOptions=PS_NONE);
	
	//Writes an error - in red.
	PS_RESULT WriteError(string Text, PS_DISPLAY_SETTINGS displayOptions=PS_NONE);

	//Writes an aligned error.
	PS_RESULT WriteError(string Text, PS_TEXT_ALIGN Align, PS_DISPLAY_SETTINGS displayOptions=PS_NONE);

	//Inserts a page break.
	PS_RESULT InsertDivide();

	//Sets the current colour to colourname.
	PS_RESULT SetColour(string ColourName);

	//Adds a hyperlink.
	PS_RESULT AddLink(string LinkText, string Link, string Colour);

private:
	CLogFile(const CLogFile& init);
	CLogFile& operator=(const CLogFile& rhs);

	bool m_IsFileOpen; //Is the file open.
	bool m_HasFrame; //Have frames been enabled.
	ofstream m_TheFile; //The main file.
	ofstream m_ErrorFile; //The error link file, used only for frames.
	string m_CurrentColour; //The current colour.
	string m_FileName; //The name of the main file.
	int m_ErrNo; //The error number.

	//Sets the current alignment of the text.
	PS_RESULT SetAlignment(PS_TEXT_ALIGN Align);

	//Writes the current line of text
	string Line(const PS_DISPLAY_SETTINGS &options);

	//Writes today's date
	string Date(const PS_DISPLAY_SETTINGS &options);
};


#endif

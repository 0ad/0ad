// last modified Thursday, May 08, 2003

#include "LogFile.h"

//-------------------------------------------------
//Add a hyperlink to Link.
//-------------------------------------------------
PS_RESULT CLogFile::AddLink(string LinkText,string Link, string Colour)
{
	if(m_IsFileOpen)
	{
		m_TheFile << "<a href=" << Link << "><font color=" << Colour << ">" << LinkText << "</font></a><br>";
	}
	else
	{
		return PS_FAIL;
	}

	return PS_OK;
}

//-------------------------------------------------
//Standard destructor.
//-------------------------------------------------
CLogFile::~CLogFile()
{
	if(m_IsFileOpen)
	{
		m_TheFile << "</font></body></html>";
		m_TheFile.close();
		if(m_HasFrame)
		{
			m_ErrorFile.close();
		}
		m_IsFileOpen = false;
	}
}

//-------------------------------------------------
//Standard constructor.
//-------------------------------------------------
CLogFile::CLogFile()
{
	m_CurrentColour = "Black";
	m_IsFileOpen = false;
}

//-------------------------------------------------
// Constructor that opens a file.
// 3rd parameter determines whether the error frame is shown.
//-------------------------------------------------
CLogFile::CLogFile(string FileName, string PageTitle, _bool WithFrame)
{
	
	Open(FileName,PageTitle,WithFrame);
	m_IsFileOpen = true;
}


//-------------------------------------------------
//Closes the open files, if open.
//-------------------------------------------------
PS_RESULT CLogFile::Close()
{
	if(m_IsFileOpen)
	{
		m_CurrentColour = "Black";
		m_TheFile << "</font></body></html>";
		m_TheFile.close();
		if(m_HasFrame)
		{
			m_ErrorFile.close();
		}
		m_IsFileOpen = false;
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Inserts a horzontal divide in the page.
//-------------------------------------------------
PS_RESULT CLogFile::InsertDivide()
{
	if(m_IsFileOpen)
	{
		m_TheFile << "<hr>";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Opens an error file. If WithFrame is true then it constructs a framed page.
//-------------------------------------------------
PS_RESULT CLogFile::Open(string FileName, string PageTitle, _bool WithFrame)
{
	if(m_IsFileOpen)
		return PS_FAIL;

	m_HasFrame=false;
	m_CurrentColour = "Black";
	//If there are no frames then create a normal page.
	if(!WithFrame)
	{
		FileName = FileName + ".html";
		m_TheFile.open(FileName.c_str());
		m_TheFile << "<html><head><title>" << PageTitle << "</title></head><body><font color = black>";
	}
	else
	{
		//Open a temporary file to write the frameset.
		ofstream TempStream;
		string TempString;
		TempString = FileName;
		TempString = FileName + ".html";
		TempStream.open(TempString.c_str());
		FileName = FileName + "b.html";

		//Write the proper details to the frame file.
		TempStream << "<html><head><title>" << PageTitle << "</title></head>";
		TempStream << "<frameset frameborder = 1 framespacing = 1 frameborder=yes border=1 rows=70,*>";
		TempStream << "<frame src=ErrorLinks.html name =title scrolling =yes><frame src="; 
		TempStream << FileName << " name =main scrolling=yes ></frameset>";
		TempStream << "</frameset></html>";
		TempStream.close();

		//Open the two pages to be displayed within the frames.
		
		m_TheFile.open(FileName.c_str());
		m_ErrorFile.open("ErrorLinks.html");
		m_FileName = FileName.c_str();
		
		//Start writing the two pages that will be displayed.
		m_TheFile << "<html><head><title>" << PageTitle << "</title></head><body><font color = black>";
		m_ErrorFile << "<html><head><title>" << PageTitle << "</title><BASE TARGET=main></head><body><font color = black>";
		m_HasFrame = true;
		m_ErrNo = 0;
	}
	
	m_IsFileOpen = true;

	return PS_OK;
}


//-------------------------------------------------
//Sets the current alignment.
//-------------------------------------------------
PS_RESULT CLogFile::SetAlignment(PS_TEXT_ALIGN Align)
{
	if(m_IsFileOpen)
	{
		switch(Align){

		case PS_ALIGN_LEFT:
			m_TheFile << "<p align = left>";
			break;
		case PS_ALIGN_CENTER:
			m_TheFile << "<p align = center>";
			break;
		case PS_ALIGN_RIGHT:
			m_TheFile << "<p align = right>";
			break;
		}
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}

//-------------------------------------------------
//Changes the current colour.
//-------------------------------------------------
PS_RESULT CLogFile::SetColour(string ColourName)
{
	if(m_IsFileOpen)
	{
		m_CurrentColour = ColourName;
		m_TheFile << "</font><font color=" << m_CurrentColour << ">";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}

//-------------------------------------------------
//Writes an error, always in red.
//-------------------------------------------------
PS_RESULT CLogFile::WriteError(string Text, PS_DISPLAY_SETTINGS displayOptions)
{
	if(m_IsFileOpen)
	{
		//If there is a frame then add an anchor and link to the appropriate places.
		if(m_HasFrame)
		{
			m_TheFile << "<a name=" << m_ErrNo << "></a>";
			m_ErrorFile << "<a href=" << m_FileName << "#" << m_ErrNo  << ">Error: " << m_ErrNo << "</a><br>";
			m_ErrNo++;
		}
		m_TheFile << "</font><font color=red>" << Line(displayOptions) << Text << Date(displayOptions) << "</font><font color =" << m_CurrentColour << "><br>";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


PS_RESULT CLogFile::WriteError(string Text, PS_TEXT_ALIGN Align, PS_DISPLAY_SETTINGS displayOptions)
{
	if(m_IsFileOpen)
	{
		SetAlignment(Align);
		//If there is a frame then add an anchor and link to the appropriate places.
		if(m_HasFrame)
		{
			m_TheFile << "<a name=" << m_ErrNo << "></a>";
			m_ErrorFile << "<a href=" << m_FileName << "#" << m_ErrNo  << ">Error: " << m_ErrNo << "</a><br>";
			m_ErrNo++;
		}
		m_TheFile << "</font><font color=red>" << Line(displayOptions) << Text << Date(displayOptions) << "</font></p><font color =" << m_CurrentColour << "><br>";
		
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Writes a header in larger text.
//-------------------------------------------------
PS_RESULT CLogFile::WriteHeader(string Text, PS_DISPLAY_SETTINGS displayOptions)
{
	
	if(m_IsFileOpen)
	{
		m_TheFile << "</font><font size=6 color= " << m_CurrentColour << ">" 
			<< Line(displayOptions) << Text << Date(displayOptions) << "</font><font size =3><br>";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Writes a header in larger text, with alignment.
//-------------------------------------------------
PS_RESULT CLogFile::WriteHeader(string Text, PS_TEXT_ALIGN Align, PS_DISPLAY_SETTINGS displayOptions)
{
	if(m_IsFileOpen)
	{
		SetAlignment(Align);
		m_TheFile << "</font><font size=6 color=" << m_CurrentColour << ">" 
			<< Line(displayOptions) << Text << Date(displayOptions) << "</font></p><font size =3><br>";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Write a normal string.
//-------------------------------------------------
PS_RESULT CLogFile::WriteText(string Text, PS_DISPLAY_SETTINGS displayOptions)
{
	if(m_IsFileOpen)
	{
		m_TheFile << Line(displayOptions) << Text << Date(displayOptions) << "<br>";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Write a normal string, with alignment.
//-------------------------------------------------
PS_RESULT CLogFile::WriteText(string Text, PS_TEXT_ALIGN Align, PS_DISPLAY_SETTINGS displayOptions)
{
	if(m_IsFileOpen)
	{	
		SetAlignment(Align);
		m_TheFile << Line(displayOptions) << Text << Date(displayOptions) << "</p><br>";
	}
	else
	{
		return PS_FAIL;
	}
	return PS_OK;
}


//-------------------------------------------------
//Retrieve a string to display the line number
//-------------------------------------------------
string CLogFile::Line(const PS_DISPLAY_SETTINGS &options)
{
	string lineText;

	if (options.displayMode == PS_DISPLAY_MODE_SHOW_LINE_NUMBER )
	{
		lineText = options.file;

		char temp[8];
		itoa(options.line, temp, 10);
		lineText += ", Line ";
		lineText += temp;
		lineText += ": ";
	}

	return lineText;
}

//-------------------------------------------------
//Retrieve a string to display the date
//-------------------------------------------------
string CLogFile::Date(const PS_DISPLAY_SETTINGS &options)
{
	string dateText;

	if (options.displayMode == PS_DISPLAY_MODE_SHOW_DATE )
	{
		dateText = "<font color=#AAAAAA> &nbsp;&nbsp;(";
		dateText += options.date;
		dateText += ")</font>";
	}

	return dateText;	
}
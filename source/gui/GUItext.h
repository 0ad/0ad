/*
GUI text, handles text stuff
by Gustav Larsson
gee@pyro.nu

--Overview--

	Mainly contains struct SGUIText and friends.
	 Actual text processing is made in CGUI::GenerateText()

--More info--

	Check GUI.h

*/

#ifndef GUItext_H
#define GUItext_H


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * @author Gustav Larsson
 *
 * An SGUIText object is a parsed string, divided into
 * text-rendering components. Each component, being a 
 * call to the Renderer. For instance, if you by tags
 * change the color, then the GUI will have to make
 * individual calls saying it want that color on the
 * text.
 *
 * For instance:
 * "Hello [b]there[/b] bunny!"
 *
 * That without word-wrapping would mean 3 components.
 * i.e. 3 calls to CRenderer. One drawing "Hello",
 * one drawing "there" in bold, and one drawing "bunny!".
 */
struct SGUIText
{
	/**
	 * @author Gustav Larsson
	 *
	 * A sprite call to the CRenderer
	 */
	struct SSpriteCall
	{
		/**
		 * Size and position of sprite
		 */
		CRect m_Area;

		/**
		 * Texture name from texture database.
		 */
		CStr m_TextureName;
	};

	/**
	 * @author Gustav Larsson
	 *
	 * A text call to the CRenderer
	 */
	struct STextCall
	{
		STextCall() : 
			m_Bold(false), m_Italic(false), m_Underlined(false),
			m_UseCustomColor(false), m_pSpriteCall(NULL) {}

		/**
		 * Position
		 */
		CPos m_Pos;

		/**
		 * Size
		 */
		CSize m_Size;

		/**
		 * The string that is suppose to be rendered.
		 */
		CStr m_String;

		/**
		 * Use custom color? If true then m_Color is used,
		 * else the color inputted will be used.
		 */
		bool m_UseCustomColor;

		/**
		 * Color setup
		 */
		CColor m_Color;

		/**
		 * Font name
		 */
		CStr m_Font;

		/**
		 * Settings
		 */
		bool m_Bold, m_Italic, m_Underlined;

		/**
		 * *IF* an icon, than this is not NULL.
		 */
		std::list<SSpriteCall>::pointer m_pSpriteCall;
	};

	/**
	 * List of TextCalls, for instance "Hello", "there!"
	 */
	std::vector<STextCall> m_TextCalls;

	/**
	 * List of sprites, or "icons" that should be rendered
	 * along with the text.
	 */
	std::list<SSpriteCall> m_SpriteCalls; // list for consistant mem addresses
										  // so that we can point to elements.

	/**
	 * Width and height of the whole output, used when setting up
	 * scrollbars and such.
	 */
	CSize m_Size;
};

/**
 * @author Gustav Larsson
 *
 * String class, substitue for CStr, but that parses
 * the tags and builds up a list of all text that will
 * be different when outputted.
 *
 * The difference between CGUIString and SGUIText is that
 * CGUIString is a string-class that parses the tags
 * when the value is set. The SGUIText is just a container
 * which stores the positions and settings of all text-calls
 * that will have to be made to the Renderer.
 */
class CGUIString
{
public:
	/**
	 * @author Gustav Larsson
	 *
	 * A chunk of text that represents one call to the renderer.
	 * In other words, all text in one chunk, will be drawn
	 * exactly with the same settings.
	 */
	struct TextChunk
	{
		/**
		 * @author Gustav Larsson
		 *
		 * A tag looks like this "Hello [B]there[/B] little"
		 */
		struct Tag
		{
			/**
			 * Tag Type
			 */
			enum TagType
			{
				TAG_B,
				TAG_I,
				TAG_FONT,
				TAG_SIZE,
				TAG_COLOR,
				TAG_IMGLEFT,
				TAG_IMGRIGHT,
				TAG_ICON
			};

			/**
			 * Set tag from string
			 *
			 * @param tagtype TagType by string, like 'IMG' for [IMG]
			 * @return True if m_TagType was set.
			 */
			bool SetTagType(const CStr& tagtype);

			/**
			 * In [B=Hello][/B]
			 * m_TagType is TAG_B
			 */
			TagType m_TagType;

			/**
			 * In [B=Hello][/B]
			 * m_TagValue is 'Hello'
			 */
			std::string m_TagValue;
		};

		/**
		 * m_From and m_To is the range of the string
		 */
		int m_From, m_To;

		/**
		 * Tags that are present. [A][B]
		 */
		std::vector<Tag> m_Tags;
	};

	/**
	 * @author Gustav Larsson
	 *
	 * All data generated in GenerateTextCall()
	 */
	struct SFeedback
	{
		// Constants
		static const int Left=0;
		static const int Right=1;

		/**
		 * Reset all member data.
		 */
		void Reset();

		/**
		 * Image stacks, for left and right floating images.
		 */
		std::vector<CStr> m_Images[2]; // left and right

		/**
		 * Text and Sprite Calls.
		 */
		std::vector<SGUIText::STextCall> m_TextCalls;
		std::list<SGUIText::SSpriteCall> m_SpriteCalls; // list for consistent mem addresses
														//  so that we can point to elements.

		/**
		 * Width and Height *feedback*
		 */
		CSize m_Size;

		/**
		 * If the word inputted was a new line.
		 */
		bool m_NewLine;
	};

	/**
	 * Set the value, the string will automatically
	 * be parsed when set.
	 */
	void SetValue(const CStr& str);

	/**
	 * Get String, without tags
	 */
	CStr GetRawString() const { return m_RawString; }

	/**
	 * Generate Text Call from specified range. The range
	 * must span only within ONE TextChunk though. Otherwise
	 * it can't be fit into a single Text Call
	 *
	 * Notice it won't make it complete, you will have to add
	 * X/Y values and such.
	 *
	 * @param Feedback contains all info that is generated.
	 * @param DefaultFont Default Font
	 * @param from From character n,
	 * @param to to chacter n.
	 * 
	 * pObject Only for Error outputting, optional! If NULL
	 * then no Errors will be reported! Useful when you need
	 * to make several GenerateTextCall in different phases,
	 * it avoids duplicates.
	 */
	void GenerateTextCall(SFeedback &Feedback,
						  const CStr& DefaultFont,
						  const int &from, const int &to,
						  const IGUIObject *pObject=NULL) const;

	/**
	 * Words
	 */
	std::vector<int> m_Words;

	/**
	 * TextChunks
	 */
	std::vector<TextChunk> m_TextChunks;

private:
	/**
	 * The full raw string. Stripped of tags.
	 */
	CStr m_RawString;
};

#endif

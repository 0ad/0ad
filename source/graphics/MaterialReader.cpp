#include "precompiled.h"
#include "ogl.h"
#include "Xeromyces.h"
#include "MaterialReader.h"

#define SAFE_DELETE(x) \
    if((x)) { delete (x); (x) = NULL; }

static SMaterialColor ParseColor(CStr colorStr)
{
    SMaterialColor color;
    
    colorStr = colorStr.Trim(PS_TRIM_BOTH);
    CStr tmp;
    int idx = 0;
    long pos = colorStr.Find(' ');
    while(colorStr.Length())
    {
        if(pos == -1)
            pos = colorStr.Length();

        tmp = colorStr.GetSubstring(0, pos);
        colorStr = colorStr.GetSubstring(pos, colorStr.Length() - pos);
        colorStr = colorStr.Trim(PS_TRIM_LEFT);
        pos = colorStr.Find(' ');

        float value = 0.0f;
        if(tmp.Find('.') != -1)
            value = tmp.ToFloat();
        else
        {
            int intValue = tmp.ToInt();
            if(intValue > 0)
            {
                if(intValue > 255)
                    intValue = 255;

                value = ((float)intValue) / 255.0f;
            }
        }

        switch(idx)
        {
        case 0:
            color.r = value;
            break;

        case 1:
            color.g = value;
            break;

        case 2:
            color.b = value;
            break;

        case 3:
            color.a = value;
            break;

        default:
            break;
        }

        if(idx >= 3)
            break;

        idx++;
    }

    return color;
}

static bool ParseUsage(CStr temp)
{
    temp = temp.LCase().Trim(PS_TRIM_BOTH);
    if(temp == CStr("blend") ||
        temp == CStr("true") ||
        temp == CStr("yes") ||
        temp.ToInt() > 0)
        return true;

    return false;
}

static GLenum ParseAlphaFunc(CStr temp)
{
    temp = temp.LCase().Trim(PS_TRIM_BOTH);
    if(!temp.Length())
        return GL_NONE;

    if(temp == CStr("never"))
        return GL_NEVER;
    else if(temp == CStr("less"))
        return GL_LESS;
    else if(temp == CStr("lequal"))
        return GL_LEQUAL;
    else if(temp == CStr("greater"))
        return GL_GREATER;
    else if(temp == CStr("gequal"))
        return GL_GEQUAL;
    else if(temp == CStr("equal"))
        return GL_EQUAL;
    else if(temp == CStr("notequal"))
        return GL_NOTEQUAL;
    else if(temp == CStr("always"))
        return GL_ALWAYS;
    else
        return GL_NONE;
}

static GLenum ParseBlendFunc(CStr temp)
{
    temp = temp.LCase().Trim(PS_TRIM_BOTH);
    if(!temp.Length())
        return GL_NONE;

    if(temp == CStr("zero"))
        return GL_ZERO;
    else if(temp == CStr("one"))
        return GL_ONE;
    else if(temp == CStr("sourcecolor"))
        return GL_SRC_COLOR;
    else if(temp == CStr("oneminussourcecolor"))
        return GL_ONE_MINUS_SRC_COLOR;
    else if(temp == CStr("destcolor"))
        return GL_DST_COLOR;
    else if(temp == CStr("oneminusdestcolor"))
        return GL_ONE_MINUS_DST_COLOR;
    else if(temp == CStr("sourcealpha"))
        return GL_SRC_ALPHA;
    else if(temp == CStr("oneminussourcealpha"))
        return GL_ONE_MINUS_SRC_ALPHA;
    else if(temp == CStr("destalpha"))
        return GL_DST_ALPHA;
    else if(temp == CStr("oneminusdestalpha"))
        return GL_ONE_MINUS_DST_ALPHA;
    else if(temp == CStr("sourcealphasaturate"))
        return GL_SRC_ALPHA_SATURATE;

    return GL_NONE;
}

static float ClampFloat(float value, float min, float max)
{
    if(value < min)
        return min;
    else if(value > max)
        return max;

    return value;
}

CMaterialReader::CMaterialReader()
{
}

CMaterialReader::~CMaterialReader()
{
}

CMaterial *CMaterialReader::LoadMaterial(const char *file)
{
	CXeromyces xeroFile;
	if (xeroFile.Load(file) != PSRETURN_OK)
		return NULL;

	#define EL(x) int el_##x = xeroFile.getElementID(#x)
	#define AT(x) int at_##x = xeroFile.getAttributeID(#x)
	EL(texture);
	
	EL(colors);
	AT(diffuse);
	AT(ambient);
	AT(specular);
	AT(emissive);
	AT(specularpower);

	EL(alpha);
	AT(usage);
	AT(function);
	AT(clamp);

	EL(blend);
	AT(sourcefunction);
	AT(destfunction);

	#undef AT
	#undef EL

	CMaterial *material = NULL;
	try
	{
		XMBElement root = xeroFile.getRoot();
		XMBElementList childNodes = root.getChildNodes();
		material = new CMaterial();

		for(i32 i = 0; i < childNodes.Count; i++)
		{
			XMBElement node = childNodes.item(i);
			int token = node.getNodeName();
            XMBAttributeList attrs = node.getAttributes();
            CStr temp;
            if(token == el_texture)
            {
                CStr value(node.getText());
                material->SetTexture(value);
            
            }
			else if(token == el_colors)
			{
				temp = (CStr)attrs.getNamedItem(at_diffuse);
				if(temp.Length() > 0)
					material->SetDiffuse(ParseColor(temp));

                temp = (CStr)attrs.getNamedItem(at_ambient);
                if(temp.Length() > 0)
                    material->SetAmbient(ParseColor(temp));

                temp = (CStr)attrs.getNamedItem(at_specular);
                if(temp.Length() > 0)
                    material->SetSpecular(ParseColor(temp));

                temp = (CStr)attrs.getNamedItem(at_specularpower);
                if(temp.Length() > 0)
                    material->SetSpecularPower(ClampFloat(temp.ToFloat(), 0.0f, 1.0f));
			}
            else if(token == el_alpha)
            {
                temp = (CStr)attrs.getNamedItem(at_usage);
                material->SetUsesAlpha(ParseUsage(temp));

                temp = (CStr)attrs.getNamedItem(at_function);
                material->SetAlphaFunc(ParseAlphaFunc(temp));

                temp = (CStr)attrs.getNamedItem(at_clamp);
                if(temp.Length() > 0)
                    material->SetAlphaClamp(ClampFloat(temp.ToFloat(), 0.0f, 1.0f));
            }
            else if(token == el_blend)
            {
                temp = (CStr)attrs.getNamedItem(at_sourcefunction);
                material->SetSourceBlend(ParseBlendFunc(temp));

                temp = (CStr)attrs.getNamedItem(at_destfunction);
                material->SetDestBlend(ParseBlendFunc(temp));
            }
		}
	}
    catch(...)
    {
        SAFE_DELETE(material);
        throw;
    }

    return material;
}
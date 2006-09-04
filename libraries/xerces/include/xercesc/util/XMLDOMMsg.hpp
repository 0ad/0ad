// This file is generated, don't edit it!!

#if !defined(ERRHEADER_XMLDOMMsg)
#define ERRHEADER_XMLDOMMsg

#include <xercesc/framework/XMLErrorReporter.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMError.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLDOMMsg
{
public :
    enum Codes
    {
        NoError                            = 0
      , F_LowBounds                        = 1
      , DOMEXCEPTION_ERRX                  = 2
      , INDEX_SIZE_ERR                     = 3
      , DOMSTRING_SIZE_ERR                 = 4
      , HIERARCHY_REQUEST_ERR              = 5
      , WRONG_DOCUMENT_ERR                 = 6
      , INVALID_CHARACTER_ERR              = 7
      , NO_DATA_ALLOWED_ERR                = 8
      , NO_MODIFICATION_ALLOWED_ERR        = 9
      , NOT_FOUND_ERR                      = 10
      , NOT_SUPPORTED_ERR                  = 11
      , INUSE_ATTRIBUTE_ERR                = 12
      , INVALID_STATE_ERR                  = 13
      , SYNTAX_ERR                         = 14
      , INVALID_MODIFICATION_ERR           = 15
      , NAMESPACE_ERR                      = 16
      , INVALID_ACCESS_ERR                 = 17
      , VALIDATION_ERR                     = 18
      , DOMRANGEEXCEPTION_ERRX             = 19
      , BAD_BOUNDARYPOINTS_ERR             = 20
      , INVALID_NODE_TYPE_ERR              = 21
      , Writer_NestedCDATA                 = 22
      , Writer_NotRepresentChar            = 23
      , Writer_NotRecognizedType           = 24
      , F_HighBounds                       = 25
      , W_LowBounds                        = 26
      , W_HighBounds                       = 27
      , E_LowBounds                        = 28
      , E_HighBounds                       = 29
    };

    static bool isFatal(const XMLDOMMsg::Codes toCheck)
    {
        return ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds));
    }

    static bool isWarning(const XMLDOMMsg::Codes toCheck)
    {
        return ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds));
    }

    static bool isError(const XMLDOMMsg::Codes toCheck)
    {
        return ((toCheck >= E_LowBounds) && (toCheck <= E_HighBounds));
    }

    static XMLErrorReporter::ErrTypes errorType(const XMLDOMMsg::Codes toCheck)
    {
       if ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds))
           return XMLErrorReporter::ErrType_Warning;
       else if ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds))
            return XMLErrorReporter::ErrType_Fatal;
       else if ((toCheck >= E_LowBounds) && (toCheck <= E_HighBounds))
            return XMLErrorReporter::ErrType_Error;
       return XMLErrorReporter::ErrTypes_Unknown;
    }
    static DOMError::ErrorSeverity  DOMErrorType(const XMLDOMMsg::Codes toCheck)
    {
       if ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds))
           return DOMError::DOM_SEVERITY_WARNING;
       else if ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds))
            return DOMError::DOM_SEVERITY_FATAL_ERROR;
       else return DOMError::DOM_SEVERITY_ERROR;
    }

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLDOMMsg();
};

XERCES_CPP_NAMESPACE_END

#endif


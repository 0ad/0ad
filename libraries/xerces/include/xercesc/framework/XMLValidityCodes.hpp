// This file is generated, don't edit it!!

#if !defined(ERRHEADER_XMLValid)
#define ERRHEADER_XMLValid

#include <xercesc/framework/XMLErrorReporter.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMError.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLValid
{
public :
    enum Codes
    {
        NoError                            = 0
      , E_LowBounds                        = 1
      , ElementNotDefined                  = 2
      , AttNotDefined                      = 3
      , NotationNotDeclared                = 4
      , RootElemNotLikeDocType             = 5
      , RequiredAttrNotProvided            = 6
      , ElementNotValidForContent          = 7
      , BadIDAttrDefType                   = 8
      , InvalidEmptyAttValue               = 9
      , ElementAlreadyExists               = 10
      , MultipleIdAttrs                    = 11
      , ReusedIDValue                      = 12
      , IDNotDeclared                      = 13
      , UnknownNotRefAttr                  = 14
      , UndeclaredElemInDocType            = 15
      , EmptyNotValidForContent            = 16
      , AttNotDefinedForElement            = 17
      , BadEntityRefAttr                   = 18
      , UnknownEntityRefAttr               = 19
      , ColonNotValidWithNS                = 20
      , NotEnoughElemsForCM                = 21
      , NoCharDataInCM                     = 22
      , DoesNotMatchEnumList               = 23
      , AttrValNotName                     = 24
      , NoMultipleValues                   = 25
      , NotSameAsFixedValue                = 26
      , RepElemInMixed                     = 27
      , NoValidatorFor                     = 28
      , IncorrectDatatype                  = 29
      , NotADatatype                       = 30
      , TextOnlyContentWithType            = 31
      , FeatureUnsupported                 = 32
      , NestedOnlyInElemOnly               = 33
      , EltRefOnlyInMixedElemOnly          = 34
      , OnlyInEltContent                   = 35
      , OrderIsAll                         = 36
      , DatatypeWithType                   = 37
      , DatatypeQualUnsupported            = 38
      , GroupContentRestricted             = 39
      , UnknownBaseDatatype                = 40
      , OneOfTypeRefArchRef                = 41
      , NoContentForRef                    = 42
      , IncorrectDefaultType               = 43
      , IllegalAttContent                  = 44
      , ValueNotInteger                    = 45
      , DatatypeError                      = 46
      , SchemaError                        = 47
      , TypeAlreadySet                     = 48
      , ProhibitedAttributePresent         = 49
      , IllegalXMLSpace                    = 50
      , NotBoolean                         = 51
      , NotDecimal                         = 52
      , FacetsInconsistent                 = 53
      , IllegalFacetValue                  = 54
      , IllegalDecimalFacet                = 55
      , UnknownFacet                       = 56
      , InvalidEnumValue                   = 57
      , OutOfBounds                        = 58
      , NotAnEnumValue                     = 59
      , NotInteger                         = 60
      , IllegalIntegerFacet                = 61
      , NotReal                            = 62
      , IllegalRealFacet                   = 63
      , ScaleLargerThanPrecision           = 64
      , PrecisionExceeded                  = 65
      , ScaleExceeded                      = 66
      , NotFloat                           = 67
      , SchemaRootError                    = 68
      , WrongTargetNamespace               = 69
      , SimpleTypeHasChild                 = 70
      , NoDatatypeValidatorForSimpleType   = 71
      , GrammarNotFound                    = 72
      , DisplayErrorMessage                = 73
      , NillNotAllowed                     = 74
      , NilAttrNotEmpty                    = 75
      , FixedDifferentFromActual           = 76
      , NoDatatypeValidatorForAttribute    = 77
      , GenericError                       = 78
      , ElementNotQualified                = 79
      , ElementNotUnQualified              = 80
      , VC_IllegalRefInStandalone          = 81
      , NoDefAttForStandalone              = 82
      , NoAttNormForStandalone             = 83
      , NoWSForStandalone                  = 84
      , VC_EntityNotFound                  = 85
      , PartialMarkupInPE                  = 86
      , DatatypeValidationFailure          = 87
      , UniqueParticleAttributionFail      = 88
      , NoAbstractInXsiType                = 89
      , NoDirectUseAbstractElement         = 90
      , NoUseAbstractType                  = 91
      , BadXsiType                         = 92
      , NonDerivedXsiType                  = 93
      , NoSubforBlock                      = 94
      , AttributeNotQualified              = 95
      , AttributeNotUnQualified            = 96
      , IC_FieldMultipleMatch              = 97
      , IC_UnknownField                    = 98
      , IC_AbsentKeyValue                  = 99
      , IC_UniqueNotEnoughValues           = 100
      , IC_KeyNotEnoughValues              = 101
      , IC_KeyRefNotEnoughValues           = 102
      , IC_KeyMatchesNillable              = 103
      , IC_DuplicateUnique                 = 104
      , IC_DuplicateKey                    = 105
      , IC_KeyRefOutOfScope                = 106
      , IC_KeyNotFound                     = 107
      , NonWSContent                       = 108
      , EmptyElemNotationAttr              = 109
      , EmptyElemHasContent                = 110
      , ElemOneNotationAttr                = 111
      , AttrDupToken                       = 112
      , ElemChildrenHasInvalidWS           = 113
      , E_HighBounds                       = 114
      , W_LowBounds                        = 115
      , W_HighBounds                       = 116
      , F_LowBounds                        = 117
      , F_HighBounds                       = 118
    };

    static bool isFatal(const XMLValid::Codes toCheck)
    {
        return ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds));
    }

    static bool isWarning(const XMLValid::Codes toCheck)
    {
        return ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds));
    }

    static bool isError(const XMLValid::Codes toCheck)
    {
        return ((toCheck >= E_LowBounds) && (toCheck <= E_HighBounds));
    }

    static XMLErrorReporter::ErrTypes errorType(const XMLValid::Codes toCheck)
    {
       if ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds))
           return XMLErrorReporter::ErrType_Warning;
       else if ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds))
            return XMLErrorReporter::ErrType_Fatal;
       else if ((toCheck >= E_LowBounds) && (toCheck <= E_HighBounds))
            return XMLErrorReporter::ErrType_Error;
       return XMLErrorReporter::ErrTypes_Unknown;
    }
    static DOMError::ErrorSeverity  DOMErrorType(const XMLValid::Codes toCheck)
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
    XMLValid();
};

XERCES_CPP_NAMESPACE_END

#endif


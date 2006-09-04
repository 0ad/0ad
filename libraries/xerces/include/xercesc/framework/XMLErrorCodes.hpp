// This file is generated, don't edit it!!

#if !defined(ERRHEADER_XMLErrs)
#define ERRHEADER_XMLErrs

#include <xercesc/framework/XMLErrorReporter.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMError.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLErrs
{
public :
    enum Codes
    {
        NoError                            = 0
      , W_LowBounds                        = 1
      , NotationAlreadyExists              = 2
      , AttListAlreadyExists               = 3
      , ContradictoryEncoding              = 4
      , UndeclaredElemInCM                 = 5
      , UndeclaredElemInAttList            = 6
      , XMLException_Warning               = 7
      , W_HighBounds                       = 8
      , E_LowBounds                        = 9
      , FeatureUnsupported                 = 10
      , TopLevelNoNameComplexType          = 11
      , TopLevelNoNameAttribute            = 12
      , NoNameRefAttribute                 = 13
      , GlobalNoNameElement                = 14
      , NoNameRefElement                   = 15
      , NoNameRefGroup                     = 16
      , NoNameRefAttGroup                  = 17
      , AnonComplexTypeWithName            = 18
      , AnonSimpleTypeWithName             = 19
      , InvalidElementContent              = 20
      , UntypedElement                     = 21
      , SimpleTypeContentError             = 22
      , ExpectedSimpleTypeInList           = 23
      , ListUnionRestrictionError          = 24
      , SimpleTypeDerivationByListError    = 25
      , ExpectedSimpleTypeInRestriction    = 26
      , DuplicateFacet                     = 27
      , ExpectedSimpleTypeInUnion          = 28
      , EmptySimpleTypeContent             = 29
      , InvalidSimpleContent               = 30
      , UnspecifiedBase                    = 31
      , InvalidComplexContent              = 32
      , SchemaElementContentError          = 33
      , ContentError                       = 34
      , UnknownSimpleType                  = 35
      , UnknownComplexType                 = 36
      , UnresolvedPrefix                   = 37
      , RefElementNotFound                 = 38
      , TypeNotFound                       = 39
      , TopLevelAttributeNotFound          = 40
      , InvalidChildInComplexType          = 41
      , BaseTypeNotFound                   = 42
      , NoAttributeInSchema                = 43
      , DatatypeValidatorCreationError     = 44
      , InvalidChildFollowingSimpleContent   = 45
      , InvalidChildFollowingConplexContent   = 46
      , InvalidComplexTypeBlockValue       = 47
      , InvalidComplexTypeFinalValue       = 48
      , AttributeDefaultFixedValue         = 49
      , NotOptionalDefaultAttValue         = 50
      , LocalAttributeWithNameRef          = 51
      , GlobalAttributeWithNameRef         = 52
      , DuplicateAttribute                 = 53
      , AttributeWithTypeAndSimpleType     = 54
      , AttributeSimpleTypeNotFound        = 55
      , ElementWithFixedAndDefault         = 56
      , DeclarationWithNameRef             = 57
      , BadAttWithRef                      = 58
      , InvalidDeclarationName             = 59
      , GlobalElementWithRef               = 60
      , ElementWithTypeAndAnonType         = 61
      , NotSimpleOrMixedElement            = 62
      , DisallowedSimpleTypeExtension      = 63
      , InvalidSimpleContentBase           = 64
      , InvalidComplexTypeBase             = 65
      , InvalidChildInSimpleContent        = 66
      , InvalidChildInComplexContent       = 67
      , AnnotationError                    = 68
      , DisallowedBaseDerivation           = 69
      , SubstitutionRepeated               = 70
      , UnionRepeated                      = 71
      , ExtensionRepeated                  = 72
      , ListRepeated                       = 73
      , RestrictionRepeated                = 74
      , InvalidBlockValue                  = 75
      , InvalidFinalValue                  = 76
      , InvalidSubstitutionGroupElement    = 77
      , SubstitutionGroupTypeMismatch      = 78
      , DuplicateElementDeclaration        = 79
      , InvalidElementBlockValue           = 80
      , InvalidElementFinalValue           = 81
      , InvalidAttValue                    = 82
      , AttributeRefContentError           = 83
      , DuplicateRefAttribute              = 84
      , ForbiddenDerivationByRestriction   = 85
      , ForbiddenDerivationByExtension     = 86
      , BaseNotComplexType                 = 87
      , ImportNamespaceDifference          = 88
      , ImportRootError                    = 89
      , DeclarationNoSchemaLocation        = 90
      , IncludeNamespaceDifference         = 91
      , OnlyAnnotationExpected             = 92
      , InvalidAttributeContent            = 93
      , AttributeRequired                  = 94
      , AttributeDisallowed                = 95
      , InvalidMin2MaxOccurs               = 96
      , AnyAttributeContentError           = 97
      , NoNameGlobalElement                = 98
      , NoCircularDefinition               = 99
      , DuplicateGlobalType                = 100
      , DuplicateGlobalDeclaration         = 101
      , WS_CollapseExpected                = 102
      , Import_1_1                         = 103
      , Import_1_2                         = 104
      , ElemIDValueConstraint              = 105
      , NoNotationType                     = 106
      , EmptiableMixedContent              = 107
      , EmptyComplexRestrictionDerivation   = 108
      , MixedOrElementOnly                 = 109
      , InvalidContentRestriction          = 110
      , ForbiddenDerivation                = 111
      , AtomicItemType                     = 112
      , MemberTypeNoUnion                  = 113
      , GroupContentError                  = 114
      , AttGroupContentError               = 115
      , MinMaxOnGroupChild                 = 116
      , DeclarationNotFound                = 117
      , AllContentLimited                  = 118
      , BadMinMaxAllCT                     = 119
      , BadMinMaxAllElem                   = 120
      , NoCircularAttGroup                 = 121
      , DuplicateAttInDerivation           = 122
      , NotExpressibleWildCardIntersection   = 123
      , BadAttDerivation_1                 = 124
      , BadAttDerivation_2                 = 125
      , BadAttDerivation_3                 = 126
      , BadAttDerivation_4                 = 127
      , BadAttDerivation_5                 = 128
      , BadAttDerivation_6                 = 129
      , BadAttDerivation_7                 = 130
      , BadAttDerivation_8                 = 131
      , BadAttDerivation_9                 = 132
      , AllContentError                    = 133
      , RedefineNamespaceDifference        = 134
      , Redefine_InvalidSimpleType         = 135
      , Redefine_InvalidSimpleTypeBase     = 136
      , Redefine_InvalidComplexType        = 137
      , Redefine_InvalidComplexTypeBase    = 138
      , Redefine_InvalidGroupMinMax        = 139
      , Redefine_DeclarationNotFound       = 140
      , Redefine_GroupRefCount             = 141
      , Redefine_AttGroupRefCount          = 142
      , Redefine_InvalidChild              = 143
      , Notation_InvalidDecl               = 144
      , Notation_DeclNotFound              = 145
      , IC_DuplicateDecl                   = 146
      , IC_BadContent                      = 147
      , IC_KeyRefReferNotFound             = 148
      , IC_KeyRefCardinality               = 149
      , IC_XPathExprMissing                = 150
      , AttUseCorrect                      = 151
      , AttDeclPropCorrect3                = 152
      , AttDeclPropCorrect5                = 153
      , AttGrpPropCorrect3                 = 154
      , InvalidTargetNSValue               = 155
      , DisplayErrorMessage                = 156
      , XMLException_Error                 = 157
      , InvalidRedefine                    = 158
      , InvalidNSReference                 = 159
      , NotAllContent                      = 160
      , InvalidAnnotationContent           = 161
      , InvalidFacetName                   = 162
      , InvalidXMLSchemaRoot               = 163
      , CircularSubsGroup                  = 164
      , SubsGroupMemberAbstract            = 165
      , ELTSchemaNS                        = 166
      , InvalidAttTNS                      = 167
      , NSDeclInvalid                      = 168
      , DOMLevel1Node                      = 169
      , E_HighBounds                       = 170
      , F_LowBounds                        = 171
      , EntityExpansionLimitExceeded       = 172
      , ExpectedCommentOrCDATA             = 173
      , ExpectedAttrName                   = 174
      , ExpectedNotationName               = 175
      , NoRepInMixed                       = 176
      , BadDefAttrDecl                     = 177
      , ExpectedDefAttrDecl                = 178
      , AttListSyntaxError                 = 179
      , ExpectedEqSign                     = 180
      , DupAttrName                        = 181
      , BadIdForXMLLangAttr                = 182
      , ExpectedElementName                = 183
      , MustStartWithXMLDecl               = 184
      , CommentsMustStartWith              = 185
      , InvalidDocumentStructure           = 186
      , ExpectedDeclString                 = 187
      , BadXMLVersion                      = 188
      , UnsupportedXMLVersion              = 189
      , UnterminatedXMLDecl                = 190
      , BadXMLEncoding                     = 191
      , BadStandalone                      = 192
      , UnterminatedComment                = 193
      , PINameExpected                     = 194
      , UnterminatedPI                     = 195
      , InvalidCharacter                   = 196
      , UnexpectedTextBeforeRoot           = 197
      , UnterminatedStartTag               = 198
      , ExpectedAttrValue                  = 199
      , UnterminatedEndTag                 = 200
      , ExpectedAttributeType              = 201
      , ExpectedEndOfTagX                  = 202
      , ExpectedMarkup                     = 203
      , NotValidAfterContent               = 204
      , ExpectedComment                    = 205
      , ExpectedCommentOrPI                = 206
      , ExpectedWhitespace                 = 207
      , NoRootElemInDOCTYPE                = 208
      , ExpectedQuotedString               = 209
      , ExpectedPublicId                   = 210
      , InvalidPublicIdChar                = 211
      , UnterminatedDOCTYPE                = 212
      , InvalidCharacterInIntSubset        = 213
      , ExpectedCDATA                      = 214
      , InvalidInitialNameChar             = 215
      , InvalidNameChar                    = 216
      , UnexpectedWhitespace               = 217
      , InvalidCharacterInAttrValue        = 218
      , ExpectedMarkupDecl                 = 219
      , TextDeclNotLegalHere               = 220
      , ConditionalSectInIntSubset         = 221
      , ExpectedPEName                     = 222
      , UnterminatedEntityDecl             = 223
      , InvalidCharacterRef                = 224
      , UnterminatedCharRef                = 225
      , ExpectedEntityRefName              = 226
      , EntityNotFound                     = 227
      , NoUnparsedEntityRefs               = 228
      , UnterminatedEntityRef              = 229
      , RecursiveEntity                    = 230
      , PartialMarkupInEntity              = 231
      , UnterminatedElementDecl            = 232
      , ExpectedContentSpecExpr            = 233
      , ExpectedAsterisk                   = 234
      , UnterminatedContentModel           = 235
      , ExpectedSystemId                   = 236
      , ExpectedSystemOrPublicId           = 237
      , UnterminatedNotationDecl           = 238
      , ExpectedSeqChoiceLeaf              = 239
      , ExpectedChoiceOrCloseParen         = 240
      , ExpectedSeqOrCloseParen            = 241
      , ExpectedEnumValue                  = 242
      , ExpectedEnumSepOrParen             = 243
      , UnterminatedEntityLiteral          = 244
      , MoreEndThanStartTags               = 245
      , ExpectedOpenParen                  = 246
      , AttrAlreadyUsedInSTag              = 247
      , BracketInAttrValue                 = 248
      , Expected2ndSurrogateChar           = 249
      , ExpectedEndOfConditional           = 250
      , ExpectedIncOrIgn                   = 251
      , ExpectedINCLUDEBracket             = 252
      , ExpectedTextDecl                   = 253
      , ExpectedXMLDecl                    = 254
      , UnexpectedEOE                      = 255
      , PEPropogated                       = 256
      , ExtraCloseSquare                   = 257
      , PERefInMarkupInIntSubset           = 258
      , EntityPropogated                   = 259
      , ExpectedNumericalCharRef           = 260
      , ExpectedOpenSquareBracket          = 261
      , BadSequenceInCharData              = 262
      , IllegalSequenceInComment           = 263
      , UnterminatedCDATASection           = 264
      , ExpectedNDATA                      = 265
      , NDATANotValidForPE                 = 266
      , HexRadixMustBeLowerCase            = 267
      , DeclStringRep                      = 268
      , DeclStringsInWrongOrder            = 269
      , NoExtRefsInAttValue                = 270
      , XMLDeclMustBeLowerCase             = 271
      , ExpectedEntityValue                = 272
      , BadDigitForRadix                   = 273
      , EndedWithTagsOnStack               = 274
      , AmbiguousContentModel              = 275
      , NestedCDATA                        = 276
      , UnknownPrefix                      = 277
      , PartialTagMarkupError              = 278
      , EmptyMainEntity                    = 279
      , CDATAOutsideOfContent              = 280
      , OnlyCharRefsAllowedHere            = 281
      , Unexpected2ndSurrogateChar         = 282
      , NoPIStartsWithXML                  = 283
      , XMLDeclMustBeFirst                 = 284
      , XMLVersionRequired                 = 285
      , StandaloneNotLegal                 = 286
      , EncodingRequired                   = 287
      , TooManyColonsInName                = 288
      , InvalidColonPos                    = 289
      , ColonNotLegalWithNS                = 290
      , SysException                       = 291
      , XMLException_Fatal                 = 292
      , UnexpectedEOF                      = 293
      , UnexpectedError                    = 294
      , BadSchemaLocation                  = 295
      , NoGrammarResolver                  = 296
      , SchemaScanFatalError               = 297
      , IllegalRefInStandalone             = 298
      , PEBetweenDecl                      = 299
      , NoEmptyStrNamespace                = 300
      , NoUseOfxmlnsAsPrefix               = 301
      , NoUseOfxmlnsURI                    = 302
      , PrefixXMLNotMatchXMLURI            = 303
      , XMLURINotMatchXMLPrefix            = 304
      , NoXMLNSAsElementPrefix             = 305
      , CT_SimpleTypeChildRequired         = 306
      , InvalidRootElemInDOCTYPE           = 307
      , InvalidElementName                 = 308
      , InvalidAttrName                    = 309
      , InvalidEntityRefName               = 310
      , F_HighBounds                       = 311
    };

    static bool isFatal(const XMLErrs::Codes toCheck)
    {
        return ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds));
    }

    static bool isWarning(const XMLErrs::Codes toCheck)
    {
        return ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds));
    }

    static bool isError(const XMLErrs::Codes toCheck)
    {
        return ((toCheck >= E_LowBounds) && (toCheck <= E_HighBounds));
    }

    static XMLErrorReporter::ErrTypes errorType(const XMLErrs::Codes toCheck)
    {
       if ((toCheck >= W_LowBounds) && (toCheck <= W_HighBounds))
           return XMLErrorReporter::ErrType_Warning;
       else if ((toCheck >= F_LowBounds) && (toCheck <= F_HighBounds))
            return XMLErrorReporter::ErrType_Fatal;
       else if ((toCheck >= E_LowBounds) && (toCheck <= E_HighBounds))
            return XMLErrorReporter::ErrType_Error;
       return XMLErrorReporter::ErrTypes_Unknown;
    }
    static DOMError::ErrorSeverity  DOMErrorType(const XMLErrs::Codes toCheck)
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
    XMLErrs();
};

XERCES_CPP_NAMESPACE_END

#endif


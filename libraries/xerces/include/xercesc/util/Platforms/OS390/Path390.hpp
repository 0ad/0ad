/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: Path390.hpp,v 1.1 2002/11/22 14:57:32 tng Exp $
 */

#ifndef PATH390_HPP
#define PATH390_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class  Path390
{
 public:
    // Constructors and Destructor
    Path390();
    ~Path390();
    Path390(char *s);

    // Set a new path in the object. This will overlay any existing path and
    // re-initialize the object.
    void setPath(char *s);

    // This performs a complete parse of the path. It returns the error code or 0
    // if there was no error.
    int  fullParse();

    // This returns the path in a format as required by fopen
    char * getfopenPath();

    // This returns the parameters in a format as required by fopen
    char * getfopenParms();

    // This returns the type of the path. See the constants defined below.
    int  getPathType();

    // This returns the error code that was found during the parse
    int  getError();

    // Returns whether the path is relative or absolute
    bool isRelative();

    // Returns whether or not type=record shows up in the path.
    bool isRecordType();

 private:
    int _pathtype;
    char * _orgpath;
    int _orglen;
    char * _resultpath;
    bool _absolute;
    bool _uriabsolute;
    bool _dsnabsolute;
    char * _curpos;
    int _parsestate;
    int _numperiods;
    int _numsemicolons;

    int _error;
    char * _orgparms;
    int _orgparmlen;

    char * _lastsemi;
    char * _lastslash;
    char * _lastparen;
    char * _parmStart;
    char * _pathEnd;
    char * _extStart;

    int _typerecord;
    // internal only methods:
    void _determine_uri_abs();
    void _determine_type();
    void _determine_punct();
    void _determine_parms();
    void _parse_rest();

};

// Internal constants for the _parsestate variable:
#define PARSE_NONE         0
#define PARSE_ABSOLUTE_URI 1
#define PARSE_PATHTYPE     2
#define PARSE_PUNCT        3
#define PARSE_PARMS        4
#define PARSE_PARSED       5

// These are the possible error return codes:
#define NO_ERROR                        0
#define ERROR_SEMICOLON_NOT_ALLOWED    101
#define ERROR_PERIOD_NOT_ALLOWED       102
#define ERROR_NO_PAREN_ALLOWED         103
#define ERROR_ABS_PATH_REQUIRED        104
#define ERROR_NO_EXTRA_PERIODS_ALLOWED 105
#define ERROR_MUST_BE_ABSOLUTE         106
#define ERROR_BAD_DD                   107
#define ERROR_BAD_DSN2                 108
#define ERROR_NO_EXTRA_SEMIS_ALLOWED   109

// Constants for the _pathtype variable and the return value from getPathType() method:
#define PATH390_HFS      1
#define PATH390_DSN1     2 // format is dsn:/chrisl/data/xml/member1.
#define PATH390_DSN2     3 // format is dsn://'chrisl.data.xml(member1)'
#define PATH390_DD       4
#define PATH390_OTHER    5

XERCES_CPP_NAMESPACE_END

#endif
